#include "FNavMeshManager.h"
#include "FDebugDrawer.h"

#include <iostream>
//#include <hash_set.h>
//#include <hash_set>
#include <set>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <algorithm>
#include "FProfiler.h"

using namespace std;


/* copyright 2016 Dr David Sinclair
david@s-hull.org

program to compute Delaunay triangulation of a set of points.

this code is released under GPL3,
a copy ofthe license can be found at
http://www.gnu.org/licenses/gpl-3.0.html

you can purchase a un-restricted licnese from
http://www.s-hull.org
for the price of one beer!

revised 2/April/2016

*/






void circle_cent2(float r1, float c1, float r2, float c2, float r3, float c3,
	float &r, float &c, float &ro2) {
	/*
	*  function to return the center of a circle and its radius
	* degenerate case should never be passed to this routine!!!!!!!!!!!!!
	* but will return r0 = -1 if it is.
	*/

	float a1 = (r1 + r2) / 2.0;
	float a2 = (c1 + c2) / 2.0;
	float b1 = (r3 + r2) / 2.0;
	float b2 = (c3 + c2) / 2.0;

	float e2 = r1 - r2;
	float e1 = -c1 + c2;

	float q2 = r3 - r2;
	float q1 = -c3 + c2;

	r = 0; c = 0; ro2 = -1;
	if (e1*-q2 + e2*q1 == 0) return;

	float beta = (-e2*(b1 - a1) + e1*(b2 - a2)) / (e2*q1 - e1*q2);

	r = b1 + q1*beta;
	c = b2 + q2*beta;

	ro2 = (r1 - r)*(r1 - r) + (c1 - c)*(c1 - c);
	return;
}


/*
read an ascii file of (r,c) point pairs.

the first line of the points file should contain
"NUMP  2 points"

if it does not have the word points in it the first line is
interpretted as a point pair.

*/

int read_Shx(std::vector<Shx> &pts, char * fname) {
	char s0[513];
	int nump = 0;
	float p1, p2;

	Shx pt;

	std::string line;
	std::string points_str("points");

	std::ifstream myfile;
	myfile.open(fname);

	if (myfile.is_open()) {

		getline(myfile, line);
		//int numc = line.length();

		// check string for the string "points"
		int n = (int)line.find(points_str);
		if (n > 0) {
			while (myfile.good()) {
				getline(myfile, line);
				if (line.length() <= 512) {
					copy(line.begin(), line.end(), s0);
					s0[line.length()] = 0;
					int v = sscanf(s0, "%g %g", &p1, &p2);
					if (v>0) {
						pt.id = nump;
						nump++;
						pt.r = p1;
						pt.c = p2;
						pts.push_back(pt);
					}
				}
			}
		}
		else {   // assume all number pairs on a line are points
			if (line.length() <= 512) {
				copy(line.begin(), line.end(), s0);
				s0[line.length()] = 0;
				int v = sscanf(s0, "%g %g", &p1, &p2);
				if (v>0) {
					pt.id = nump;
					nump++;
					pt.r = p1;
					pt.c = p2;
					pts.push_back(pt);
				}
			}

			while (myfile.good()) {
				getline(myfile, line);
				if (line.length() <= 512) {
					copy(line.begin(), line.end(), s0);
					s0[line.length()] = 0;
					int v = sscanf(s0, "%g %g", &p1, &p2);
					if (v>0) {
						pt.id = nump;
						nump++;
						pt.r = p1;
						pt.c = p2;
						pts.push_back(pt);
					}
				}
			}
		}
		myfile.close();
	}

	nump = (int)pts.size();

	return(nump);
};

/*
write out a set of points to disk


*/

void write_Shx(std::vector<Shx> &pts, char * fname) {
	std::ofstream out(fname, ios::out);

	int nr = (int)pts.size();
	out << nr << " 2 points" << endl;

	for (int r = 0; r < nr; r++) {
		out << pts[r].r << ' ' << pts[r].c << endl;
	}
	out.close();

	return;
};



/*
write out triangle ids to be compatible with matlab/octave array numbering.

*/
void write_Triads(std::vector<Triad> &ts, char * fname) {
	std::ofstream out(fname, ios::out);

	int nr = (int)ts.size();
	out << nr << " 6   point-ids (1,2,3)  adjacent triangle-ids ( limbs ab  ac  bc )" << endl;

	for (int r = 0; r < nr; r++) {
		out << ts[r].a + 1 << ' ' << ts[r].b + 1 << ' ' << ts[r].c + 1 << ' '
			<< ts[r].ab + 1 << ' ' << ts[r].ac + 1 << ' ' << ts[r].bc + 1 << endl; //" " << ts[r].ro <<  endl;
	}
	out.close();

	return;
};





/*  version in which the ids of the triangles associated with the sides of the hull are tracked.


*/

int s_hull_pro(std::vector<Shx> &pts, std::vector<Triad> &triads)
{

	int nump = (int)pts.size();


	if (nump < 3) {
		cerr << "less than 3 points, aborting " << endl;
		return(-1);
	}


	float r = pts[0].r;
	float c = pts[0].c;
	for (int k = 0; k<nump; k++) {
		float dr = pts[k].r - r;
		float dc = pts[k].c - c;

		pts[k].ro = dr*dr + dc*dc;

	}

	sort(pts.begin(), pts.end());


	float r1 = pts[0].r;
	float c1 = pts[0].c;

	float r2 = pts[1].r;
	float c2 = pts[1].c;
	int mid = -1;
	float romin2 = 9.0e20, ro2, R, C;

	int k = 2;
	while (k<nump) {

		circle_cent2(r1, c1, r2, c2, pts[k].r, pts[k].c, r, c, ro2);
		if (ro2 < romin2 && ro2 > 0) {
			mid = k;
			romin2 = ro2;
			R = r;
			C = c;

		}
		else if (romin2 * 4 < pts[k].ro)
			k = nump;

		k++;
	}

	if (mid < 0) {
		cerr << "linear structure, aborting " << endl;
		return(-2);
	}


	Shx pt0 = pts[0];
	Shx pt1 = pts[1];
	Shx pt2 = pts[mid];

	int ptest = test_center(pt0, pt1, pt2);
	if (ptest < 0) {
		cerr << "warning: obtuce seed triangle sellected " << endl;
	}


	pts.erase(pts.begin() + mid);  // necessary for round off reasons:((((((
	pts.erase(pts.begin());
	pts.erase(pts.begin());

	for (int k = 0; k<nump - 3; k++) {
		float dr = pts[k].r - R;
		float dc = pts[k].c - C;

		pts[k].ro = dr*dr + dc*dc;

	}

	sort(pts.begin(), pts.end());

	pts.insert(pts.begin(), pt2);
	pts.insert(pts.begin(), pt1);
	pts.insert(pts.begin(), pt0);

	std::vector<int> slump;
	slump.resize(nump);

	for (int k = 0; k<nump; k++) {
		if (pts[k].id < nump) {
			slump[pts[k].id] = k;
		}
		else {
			int mx = pts[k].id + 1;
			while ((int)slump.size() <= mx) {
				slump.push_back(0);
			}
			slump[pts[k].id] = k;
		}
	}

	std::vector<Shx> hull;


	r = (pts[0].r + pts[1].r + pts[2].r) / (float) 3.0;
	c = (pts[0].c + pts[1].c + pts[2].c) / (float) 3.0;

	float dr0 = pts[0].r - r, dc0 = pts[0].c - c;
	float tr01 = pts[1].r - pts[0].r, tc01 = pts[1].c - pts[0].c;

	float df = -tr01* dc0 + tc01*dr0;
	if (df < 0) {   // [ 0 1 2 ]
		pt0.tr = pt1.r - pt0.r;
		pt0.tc = pt1.c - pt0.c;
		pt0.trid = 0;
		hull.push_back(pt0);

		pt1.tr = pt2.r - pt1.r;
		pt1.tc = pt2.c - pt1.c;
		pt1.trid = 0;
		hull.push_back(pt1);

		pt2.tr = pt0.r - pt2.r;
		pt2.tc = pt0.c - pt2.c;
		pt2.trid = 0;
		hull.push_back(pt2);


		Triad tri(pt0.id, pt1.id, pt2.id);
		tri.ro = romin2;
		tri.R = R;
		tri.C = C;

		triads.push_back(tri);

	}
	else {          // [ 0 2 1 ] as anti-clockwise turning is the work of the devil....
		pt0.tr = pt2.r - pt0.r;
		pt0.tc = pt2.c - pt0.c;
		pt0.trid = 0;
		hull.push_back(pt0);

		pt2.tr = pt1.r - pt2.r;
		pt2.tc = pt1.c - pt2.c;
		pt2.trid = 0;
		hull.push_back(pt2);

		pt1.tr = pt0.r - pt1.r;
		pt1.tc = pt0.c - pt1.c;
		pt1.trid = 0;
		hull.push_back(pt1);

		Triad tri(pt0.id, pt2.id, pt1.id);
		tri.ro = romin2;
		tri.R = R;
		tri.C = C;
		triads.push_back(tri);
	}

	// add new points into hull (removing obscured ones from the chain)
	// and creating triangles....
	// that will need to be flipped.

	float dr, dc, rx, cx;
	Shx  ptx;
	int numt;

	//  write_Triads(triads, "rose_0.mat");

	for (int k = 3; k<nump; k++) {
		rx = pts[k].r;    cx = pts[k].c;
		ptx.r = rx;
		ptx.c = cx;
		ptx.id = pts[k].id;

		int numh = (int)hull.size(), numh_old = numh;
		dr = rx - hull[0].r;    dc = cx - hull[0].c;  // outwards pointing from hull[0] to pt.

		std::vector<int> pidx, tridx;
		int hidx;  // new hull point location within hull.....


		float df = -dc* hull[0].tr + dr*hull[0].tc;    // visibility test vector.
		if (df < 0) {  // starting with a visible hull facet !!!
			int e1 = 1, e2 = numh;
			hidx = 0;

			// check to see if segment numh is also visible
			df = -dc* hull[numh - 1].tr + dr*hull[numh - 1].tc;
			//cerr << df << ' ' ;
			if (df < 0) {    // visible.
				pidx.push_back(hull[numh - 1].id);
				tridx.push_back(hull[numh - 1].trid);


				for (int h = 0; h<numh - 1; h++) {
					// if segment h is visible delete h
					dr = rx - hull[h].r;    dc = cx - hull[h].c;
					df = -dc* hull[h].tr + dr*hull[h].tc;
					pidx.push_back(hull[h].id);
					tridx.push_back(hull[h].trid);
					if (df < 0) {
						hull.erase(hull.begin() + h);
						h--;
						numh--;
					}
					else {	  // quit on invisibility
						ptx.tr = hull[h].r - ptx.r;
						ptx.tc = hull[h].c - ptx.c;

						hull.insert(hull.begin(), ptx);
						numh++;
						break;
					}
				}
				// look backwards through the hull structure.

				for (int h = numh - 2; h>0; h--) {
					// if segment h is visible delete h + 1
					dr = rx - hull[h].r;    dc = cx - hull[h].c;
					df = -dc* hull[h].tr + dr*hull[h].tc;

					if (df < 0) {  // h is visible 
						pidx.insert(pidx.begin(), hull[h].id);
						tridx.insert(tridx.begin(), hull[h].trid);
						hull.erase(hull.begin() + h + 1);  // erase end of chain

					}
					else {

						h = (int)hull.size() - 1;
						hull[h].tr = -hull[h].r + ptx.r;   // points at start of chain.
						hull[h].tc = -hull[h].c + ptx.c;
						break;
					}
				}

				df = 9;

			}
			else {
				//	cerr << df << ' ' << endl;
				hidx = 1;  // keep pt hull[0]
				tridx.push_back(hull[0].trid);
				pidx.push_back(hull[0].id);

				for (int h = 1; h<numh; h++) {
					// if segment h is visible delete h  
					dr = rx - hull[h].r;    dc = cx - hull[h].c;
					df = -dc* hull[h].tr + dr*hull[h].tc;
					pidx.push_back(hull[h].id);
					tridx.push_back(hull[h].trid);
					if (df < 0) {                     // visible
						hull.erase(hull.begin() + h);
						h--;
						numh--;
					}
					else {	  // quit on invisibility
						ptx.tr = hull[h].r - ptx.r;
						ptx.tc = hull[h].c - ptx.c;

						hull[h - 1].tr = ptx.r - hull[h - 1].r;
						hull[h - 1].tc = ptx.c - hull[h - 1].c;

						hull.insert(hull.begin() + h, ptx);
						break;
					}
				}
			}

			df = 8;

		}
		else {
			int e1 = -1, e2 = numh;
			for (int h = 1; h<numh; h++) {
				dr = rx - hull[h].r;    dc = cx - hull[h].c;
				df = -dc* hull[h].tr + dr*hull[h].tc;
				if (df < 0) {
					if (e1 < 0) e1 = h;  // fist visible
				}
				else {
					if (e1 > 0) { // first invisible segment.
						e2 = h;
						break;
					}
				}

			}


			// triangle pidx starts at e1 and ends at e2 (inclusive).	
			if (e2 < numh) {
				for (int e = e1; e <= e2; e++) {
					pidx.push_back(hull[e].id);
					tridx.push_back(hull[e].trid);
				}
			}
			else {
				for (int e = e1; e<e2; e++) {
					pidx.push_back(hull[e].id);
					tridx.push_back(hull[e].trid);   // there are only n-1 triangles from n hull pts.
				}
				pidx.push_back(hull[0].id);
			}


			// erase elements e1+1 : e2-1 inclusive.

			if (e1 < e2 - 1) {
				hull.erase(hull.begin() + e1 + 1, hull.begin() + e2);
			}

			// insert ptx at location e1+1.
			if (e2 == numh) {
				ptx.tr = hull[0].r - ptx.r;
				ptx.tc = hull[0].c - ptx.c;
			}
			else {
				ptx.tr = hull[e1 + 1].r - ptx.r;
				ptx.tc = hull[e1 + 1].c - ptx.c;
			}

			hull[e1].tr = ptx.r - hull[e1].r;
			hull[e1].tc = ptx.c - hull[e1].c;

			hull.insert(hull.begin() + e1 + 1, ptx);
			hidx = e1 + 1;

		}


		int a = ptx.id, T0;
		Triad trx(a, 0, 0);
		r1 = pts[slump[a]].r;
		c1 = pts[slump[a]].c;

		int npx = (int)pidx.size() - 1;
		numt = (int)triads.size();
		T0 = numt;

		if (npx == 1) {
			trx.b = pidx[0];
			trx.c = pidx[1];

			trx.bc = tridx[0];
			trx.ab = -1;
			trx.ac = -1;

			// index back into the triads.
			Triad &txx = triads[tridx[0]];
			if ((trx.b == txx.a && trx.c == txx.b) | (trx.b == txx.b && trx.c == txx.a)) {
				txx.ab = numt;
			}
			else if ((trx.b == txx.a && trx.c == txx.c) | (trx.b == txx.c && trx.c == txx.a)) {
				txx.ac = numt;
			}
			else if ((trx.b == txx.b && trx.c == txx.c) | (trx.b == txx.c && trx.c == txx.b)) {
				txx.bc = numt;
			}


			hull[hidx].trid = numt;
			if (hidx > 0)
				hull[hidx - 1].trid = numt;
			else {
				numh = (int)hull.size();
				hull[numh - 1].trid = numt;
			}
			triads.push_back(trx);
			numt++;
		}

		else {
			trx.ab = -1;
			for (int p = 0; p<npx; p++) {
				trx.b = pidx[p];
				trx.c = pidx[p + 1];


				trx.bc = tridx[p];
				if (p > 0)
					trx.ab = numt - 1;
				trx.ac = numt + 1;

				// index back into the triads.
				Triad &txx = triads[tridx[p]];
				if ((trx.b == txx.a && trx.c == txx.b) | (trx.b == txx.b && trx.c == txx.a)) {
					txx.ab = numt;
				}
				else if ((trx.b == txx.a && trx.c == txx.c) | (trx.b == txx.c && trx.c == txx.a)) {
					txx.ac = numt;
				}
				else if ((trx.b == txx.b && trx.c == txx.c) | (trx.b == txx.c && trx.c == txx.b)) {
					txx.bc = numt;
				}

				triads.push_back(trx);
				numt++;
			}
			triads[numt - 1].ac = -1;

			hull[hidx].trid = numt - 1;
			if (hidx > 0)
				hull[hidx - 1].trid = T0;
			else {
				numh = (int)hull.size();
				hull[numh - 1].trid = T0;
			}


		}

		/*
		char tname[128];
		sprintf(tname,"rose_%d.mat",k);
		write_Triads(triads, tname);
		int dbgb = 0;
		*/

	}

	cerr << "of triangles " << triads.size() << " to be flipped. " << endl;

	//  write_Triads(triads, "tris0.mat");
	
	std::vector<int> ids, ids2;

	int tf = T_flip_pro(pts, triads, slump, numt, 0, ids);
	if (tf < 0) {
		cerr << "cannot triangualte this set " << endl;

		return(-3);
	}

	//  write_Triads(triads, "tris1.mat");

	// cerr << "n-ids " << ids.size() << endl;


	int nits = (int)ids.size(), nit = 1;
	while (nits > 0 && nit < 50) {

		tf = T_flip_pro_idx(pts, triads, slump, ids, ids2);
		nits = (int)ids2.size();
		ids.swap(ids2);

		// cerr << "flipping cycle  " << nit << "   active triangles " << nits << endl;

		nit++;
		if (tf < 0) {
			cerr << "cannot triangualte this set " << endl;

			return(-4);
		}
	}

	ids.clear();
	nits = T_flip_edge(pts, triads, slump, numt, 0, ids);
	nit = 0;


	while (nits > 0 && nit < 100) {

		tf = T_flip_pro_idx(pts, triads, slump, ids, ids2);
		ids.swap(ids2);
		nits = (int)ids.size();
		//cerr << "flipping cycle  " << nit << "   active triangles " << nits << endl;

		nit++;
		if (tf < 0) {
			cerr << "cannot triangualte this set " << endl;

			return(-4);
		}
	}
	return(1);
}


void circle_cent4(float r1, float c1, float r2, float c2, float r3, float c3,
	float &r, float &c, float &ro2) {
	/*
	*  function to return the center of a circle and its radius
	* degenerate case should never be passed to this routine!!!!!!!!!!!!!
	* but will return r0 = -1 if it is.
	*/

	double rd, cd;
	double v1 = 2 * (r2 - r1), v2 = 2 * (c2 - c1), v3 = r2*r2 - r1*r1 + c2*c2 - c1*c1;
	double v4 = 2 * (r3 - r1),
		v5 = 2 * (c3 - c1),
		v6 = r3*r3 - r1*r1 + c3*c3 - c1*c1,

		v7 = v2*v4 - v1*v5;
	if (v7 == 0) {
		r = 0;
		c = 0;
		ro2 = -1;
		return;
	}

	cd = (v4*v3 - v1*v6) / v7;
	if (v1 != 0)
		rd = (v3 - c*v2) / v1;
	else
		rd = (v6 - c*v5) / v4;

	ro2 = (float)((rd - r1)*(rd - r1) + (cd - c1)*(cd - c1));
	r = (float)rd;
	c = (float)cd;

	return;
}


/* test a set of points for duplicates.

erase duplicate points, do not change point ids.

*/

int de_duplicate(std::vector<Shx> &pts, std::vector<int> &outx) {

	int nump = (int)pts.size();
	std::vector<Dupex> dpx;
	Dupex d;
	for (int k = 0; k<nump; k++) {
		d.r = pts[k].r;
		d.c = pts[k].c;
		d.id = k;
		dpx.push_back(d);
	}

	sort(dpx.begin(), dpx.end());

	for (int k = 0; k<nump - 1; k++) {
		if (dpx[k].r == dpx[k + 1].r && dpx[k].c == dpx[k + 1].c) {
			cerr << "duplicate-point ids " << dpx[k].id << "  " << dpx[k + 1].id << "   at  (" << pts[dpx[k + 1].id].r << "," << pts[dpx[k + 1].id].c << ")" << endl;
			outx.push_back(dpx[k + 1].id);
		}
	}

	if (outx.size() == 0)
		return(0);

	sort(outx.begin(), outx.end());

	int nx = (int)outx.size();
	for (int k = nx - 1; k >= 0; k--) {
		pts.erase(pts.begin() + outx[k]);
	}

	return(nx);
}




/*
flip pairs of triangles that are not valid delaunay triangles
the Cline-Renka test is used rather than the less stable circum
circle center computation test of s-hull.

or the more expensive determinant test.

*/


int T_flip_pro(std::vector<Shx> &pts, std::vector<Triad> &triads, std::vector<int> &slump, int numt, int start, std::vector<int> &ids) {

	float r3, c3;
	int pa, pb, pc, pd, D, L1, L2, L3, L4, T2;

	Triad tx, tx2;


	for (int t = start; t<numt; t++) {

		Triad &tri = triads[t];
		// test all 3 neighbours of tri 

		int flipped = 0;

		if (tri.bc >= 0) {

			pa = slump[tri.a];
			pb = slump[tri.b];
			pc = slump[tri.c];

			T2 = tri.bc;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.b == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.b == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.b == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}


			if (pd < 0 || pd > 100)
				int dfx = 9;

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pa].r, pts[pa].c, pts[pb].r, pts[pb].c,
				pts[pc].r, pts[pc].c, r3, c3);

			if (XX < 0) {

				L1 = tri.ab;
				L2 = tri.ac;
				if (L1 != L3 && L2 != L4) {  // need this check for stability.

					tx.a = tri.a;
					tx.b = tri.b;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.a;
					tx2.b = tri.c;
					tx2.c = D;

					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;


					ids.push_back(t);
					ids.push_back(T2);

					t2 = tx2;
					tri = tx;
					flipped = 1;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}
				}
			}
		}


		if (flipped == 0 && tri.ab >= 0) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ab;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pc].r, pts[pc].c, pts[pb].r, pts[pb].c,
				pts[pa].r, pts[pa].c, r3, c3);

			if (XX < 0) {


				L1 = tri.ac;
				L2 = tri.bc;
				if (L1 != L3 && L2 != L4) {  // need this check for stability.

					tx.a = tri.c;
					tx.b = tri.a;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.c;
					tx2.b = tri.b;
					tx2.c = D;

					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;


					ids.push_back(t);
					ids.push_back(T2);

					t2 = tx2;
					tri = tx;
					flipped = 1;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}

				}

			}
		}


		if (flipped == 0 && tri.ac >= 0) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ac;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pb].r, pts[pb].c, pts[pa].r, pts[pa].c,
				pts[pc].r, pts[pc].c, r3, c3);

			if (XX < 0) {

				L1 = tri.ab;   // .ac shared limb
				L2 = tri.bc;
				if (L1 != L3 && L2 != L4) {  // need this check for stability.

					tx.a = tri.b;
					tx.b = tri.a;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.b;
					tx2.b = tri.c;
					tx2.c = D;

					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;

					ids.push_back(t);
					ids.push_back(T2);

					t2 = tx2;
					tri = tx;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}

				}
			}
		}


	}


	return(1);
}

/* minimum angle cnatraint for circum circle test.
due to Cline & Renka

A   --    B

|    /    |

C   --    D


*/

int Cline_Renka_test(float &Ax, float &Ay,
	float &Bx, float &By,
	float &Cx, float &Cy,
	float &Dx, float &Dy)
{

	float v1x = Bx - Ax, v1y = By - Ay, v2x = Cx - Ax, v2y = Cy - Ay,
		v3x = Bx - Dx, v3y = By - Dy, v4x = Cx - Dx, v4y = Cy - Dy;
	float cosA = v1x*v2x + v1y*v2y;
	float cosD = v3x*v4x + v3y*v4y;

	if (cosA < 0 && cosD < 0) // two obtuse angles 
		return(-1);

	float ADX = Ax - Dx, ADy = Ay - Dy;


	if (cosA > 0 && cosD > 0)  // two acute angles
		return(1);


	float sinA = fabs(v1x*v2y - v1y*v2x);
	float sinD = fabs(v3x*v4y - v3y*v4x);

	if (cosA*sinD + sinA*cosD < 0)
		return(-1);

	return(1);

}




// same again but with set of triangle ids to be iterated over.


int T_flip_pro_idx(std::vector<Shx> &pts, std::vector<Triad> &triads, std::vector<int> &slump,
	std::vector<int> &ids, std::vector<int> &ids2) {

	float  r3, c3;
	int pa, pb, pc, pd, D, L1, L2, L3, L4, T2;

	Triad tx, tx2;
	ids2.clear();
	//std::vector<int> ids2;

	int numi = ids.size();

	for (int x = 0; x<numi; x++) {
		int t = ids[x];


		Triad &tri = triads[t];
		// test all 3 neighbours of tri 
		int flipped = 0;



		if (tri.bc >= 0) {

			pa = slump[tri.a];
			pb = slump[tri.b];
			pc = slump[tri.c];

			T2 = tri.bc;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.b == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.b == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.b == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << "  T2: " << T2 << endl;
				return(-6);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pa].r, pts[pa].c, pts[pb].r, pts[pb].c,
				pts[pc].r, pts[pc].c, r3, c3);

			if (XX < 0) {
				L1 = tri.ab;
				L2 = tri.ac;

				if (L1 != L3 && L2 != L4) {  // need this check for stability.


					tx.a = tri.a;
					tx.b = tri.b;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.a;
					tx2.b = tri.c;
					tx2.c = D;

					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;

					ids2.push_back(t);
					ids2.push_back(T2);

					t2 = tx2;
					tri = tx;
					flipped = 1;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}

				}
			}
		}


		if (flipped == 0 && tri.ab >= 0) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ab;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-6);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pc].r, pts[pc].c, pts[pb].r, pts[pb].c,
				pts[pa].r, pts[pa].c, r3, c3);

			if (XX < 0) {
				L1 = tri.ac;
				L2 = tri.bc;
				if (L1 != L3 && L2 != L4) {  // need this check for stability.

					tx.a = tri.c;
					tx.b = tri.a;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.c;
					tx2.b = tri.b;
					tx2.c = D;

					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;


					ids2.push_back(t);
					ids2.push_back(T2);

					t2 = tx2;
					tri = tx;
					flipped = 1;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}

				}
			}
		}


		if (flipped == 0 && tri.ac >= 0) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ac;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-6);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pb].r, pts[pb].c, pts[pc].r, pts[pc].c,
				pts[pa].r, pts[pa].c, r3, c3);

			if (XX < 0) {
				L1 = tri.ab;   // .ac shared limb
				L2 = tri.bc;
				if (L1 != L3 && L2 != L4) {  // need this check for stability.


					tx.a = tri.b;
					tx.b = tri.a;
					tx.c = D;

					tx.ab = L1;
					tx.ac = T2;
					tx.bc = L3;


					// triangle 2;
					tx2.a = tri.b;
					tx2.b = tri.c;
					tx2.c = D;



					tx2.ab = L2;
					tx2.ac = t;
					tx2.bc = L4;


					ids2.push_back(t);
					ids2.push_back(T2);

					t2 = tx2;
					tri = tx;

					// change knock on triangle labels.
					if (L3 >= 0) {
						Triad &t3 = triads[L3];
						if (t3.ab == T2) t3.ab = t;
						else if (t3.bc == T2) t3.bc = t;
						else if (t3.ac == T2) t3.ac = t;
					}

					if (L2 >= 0) {
						Triad &t4 = triads[L2];
						if (t4.ab == t) t4.ab = T2;
						else if (t4.bc == t) t4.bc = T2;
						else if (t4.ac == t) t4.ac = T2;
					}


				}
			}
		}
	}

	/*
	if( ids2.size() > 5){
	sort(ids2.begin(), ids2.end());
	int nums = ids2.size();
	int last = ids2[0], n=0;
	ids3.push_back(last);
	for(int g=1; g<nums; g++){
	n = ids2[g];
	if( n != last ){
	ids3.push_back(n);
	last = n;
	}
	}
	}
	else{
	int nums = ids2.size();
	for(int g=1; g<nums; g++){
	ids3.push_back(ids2[g]);
	}
	} */


	return(1);
}

/* test the seed configuration to see if the center
of the circum circle lies inside the seed triangle.

if not issue a warning.
*/


int  test_center(Shx &pt0, Shx &pt1, Shx &pt2) {

	float r01 = pt1.r - pt0.r;
	float c01 = pt1.c - pt0.c;

	float r02 = pt2.r - pt0.r;
	float c02 = pt2.c - pt0.c;

	float r21 = pt1.r - pt2.r;
	float c21 = pt1.c - pt2.c;

	float v = r01*r02 + c01*c02;
	if (v < 0) return(-1);

	v = r21*r02 + c21*c02;
	if (v > 0) return(-1);

	v = r01*r21 + c01*c21;
	if (v < 0) return(-1);

	return(1);
}

int de_duplicateX(std::vector<Shx> &pts, std::vector<int> &outx, std::vector<Shx> &pts2) {

	int nump = (int)pts.size();
	std::vector<Dupex> dpx;
	Dupex d;
	for (int k = 0; k<nump; k++) {
		d.r = pts[k].r;
		d.c = pts[k].c;
		d.id = k;
		dpx.push_back(d);
	}

	sort(dpx.begin(), dpx.end());

	cerr << "de-duplicating ";  pts2.clear();
	pts2.push_back(pts[dpx[0].id]);
	pts2[0].id = 0;
	int cnt = 1;

	for (int k = 0; k<nump - 1; k++) {
		if (dpx[k].r == dpx[k + 1].r && dpx[k].c == dpx[k + 1].c) {
			//cerr << "duplicate-point ids " << dpx[k].id << "  " << dpx[k+1].id << "   at  ("  << pts[dpx[k+1].id].r << "," << pts[dpx[k+1].id].c << ")" << endl;
			//cerr << dpx[k+1].id << " ";

			outx.push_back(dpx[k + 1].id);
		}
		else {
			pts[dpx[k + 1].id].id = cnt;
			pts2.push_back(pts[dpx[k + 1].id]);
			cnt++;
		}
	}

	cerr << "removed  " << outx.size() << endl;

	return(outx.size());
}



int T_flip_edge(std::vector<Shx> &pts, std::vector<Triad> &triads, std::vector<int> &slump, int numt, int start, std::vector<int> &ids) {

	float r3, c3;
	int pa, pb, pc, pd, D, L1, L2, L3, L4, T2;

	Triad tx, tx2;


	for (int t = start; t<numt; t++) {

		Triad &tri = triads[t];
		// test all 3 neighbours of tri 

		int flipped = 0;

		if (tri.bc >= 0 && (tri.ac < 0 || tri.ab < 0)) {

			pa = slump[tri.a];
			pb = slump[tri.b];
			pc = slump[tri.c];

			T2 = tri.bc;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.b == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.b == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.b == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}


			if (pd < 0 || pd > 100)
				int dfx = 9;

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pa].r, pts[pa].c, pts[pb].r, pts[pb].c,
				pts[pc].r, pts[pc].c, r3, c3);

			if (XX < 0) {

				L1 = tri.ab;
				L2 = tri.ac;
				//	if( L1 != L3 && L2 != L4 ){  // need this check for stability.

				tx.a = tri.a;
				tx.b = tri.b;
				tx.c = D;

				tx.ab = L1;
				tx.ac = T2;
				tx.bc = L3;


				// triangle 2;
				tx2.a = tri.a;
				tx2.b = tri.c;
				tx2.c = D;

				tx2.ab = L2;
				tx2.ac = t;
				tx2.bc = L4;


				ids.push_back(t);
				ids.push_back(T2);

				t2 = tx2;
				tri = tx;
				flipped = 1;

				// change knock on triangle labels.
				if (L3 >= 0) {
					Triad &t3 = triads[L3];
					if (t3.ab == T2) t3.ab = t;
					else if (t3.bc == T2) t3.bc = t;
					else if (t3.ac == T2) t3.ac = t;
				}

				if (L2 >= 0) {
					Triad &t4 = triads[L2];
					if (t4.ab == t) t4.ab = T2;
					else if (t4.bc == t) t4.bc = T2;
					else if (t4.ac == t) t4.ac = T2;
				}
				//	}
			}
		}


		if (flipped == 0 && tri.ab >= 0 && (tri.ac < 0 || tri.bc < 0)) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ab;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pc].r, pts[pc].c, pts[pb].r, pts[pb].c,
				pts[pa].r, pts[pa].c, r3, c3);

			if (XX < 0) {


				L1 = tri.ac;
				L2 = tri.bc;
				//	if( L1 != L3 && L2 != L4 ){  // need this check for stability.

				tx.a = tri.c;
				tx.b = tri.a;
				tx.c = D;

				tx.ab = L1;
				tx.ac = T2;
				tx.bc = L3;


				// triangle 2;
				tx2.a = tri.c;
				tx2.b = tri.b;
				tx2.c = D;

				tx2.ab = L2;
				tx2.ac = t;
				tx2.bc = L4;


				ids.push_back(t);
				ids.push_back(T2);

				t2 = tx2;
				tri = tx;
				flipped = 1;

				// change knock on triangle labels.
				if (L3 >= 0) {
					Triad &t3 = triads[L3];
					if (t3.ab == T2) t3.ab = t;
					else if (t3.bc == T2) t3.bc = t;
					else if (t3.ac == T2) t3.ac = t;
				}

				if (L2 >= 0) {
					Triad &t4 = triads[L2];
					if (t4.ab == t) t4.ab = T2;
					else if (t4.bc == t) t4.bc = T2;
					else if (t4.ac == t) t4.ac = T2;
				}

				//	}

			}
		}


		if (flipped == 0 && tri.ac >= 0 && (tri.bc < 0 || tri.ab < 0)) {

			pc = slump[tri.c];
			pb = slump[tri.b];
			pa = slump[tri.a];

			T2 = tri.ac;
			Triad &t2 = triads[T2];
			// find relative orientation (shared limb).
			if (t2.ab == t) {
				D = t2.c;
				pd = slump[t2.c];

				if (tri.a == t2.a) {
					L3 = t2.ac;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ac;
				}
			}
			else if (t2.ac == t) {
				D = t2.b;
				pd = slump[t2.b];

				if (tri.a == t2.a) {
					L3 = t2.ab;
					L4 = t2.bc;
				}
				else {
					L3 = t2.bc;
					L4 = t2.ab;
				}
			}
			else if (t2.bc == t) {
				D = t2.a;
				pd = slump[t2.a];

				if (tri.a == t2.b) {
					L3 = t2.ab;
					L4 = t2.ac;
				}
				else {
					L3 = t2.ac;
					L4 = t2.ab;
				}
			}
			else {
				cerr << "triangle flipping error. " << t << endl;
				return(-5);
			}

			r3 = pts[pd].r;
			c3 = pts[pd].c;

			int XX = Cline_Renka_test(pts[pb].r, pts[pb].c, pts[pa].r, pts[pa].c,
				pts[pc].r, pts[pc].c, r3, c3);

			if (XX < 0) {

				L1 = tri.ab;   // .ac shared limb
				L2 = tri.bc;
				//	if( L1 != L3 && L2 != L4 ){  // need this check for stability.

				tx.a = tri.b;
				tx.b = tri.a;
				tx.c = D;

				tx.ab = L1;
				tx.ac = T2;
				tx.bc = L3;


				// triangle 2;
				tx2.a = tri.b;
				tx2.b = tri.c;
				tx2.c = D;

				tx2.ab = L2;
				tx2.ac = t;
				tx2.bc = L4;

				ids.push_back(t);
				ids.push_back(T2);

				t2 = tx2;
				tri = tx;

				// change knock on triangle labels.
				if (L3 >= 0) {
					Triad &t3 = triads[L3];
					if (t3.ab == T2) t3.ab = t;
					else if (t3.bc == T2) t3.bc = t;
					else if (t3.ac == T2) t3.ac = t;
				}

				if (L2 >= 0) {
					Triad &t4 = triads[L2];
					if (t4.ab == t) t4.ab = T2;
					else if (t4.bc == t) t4.bc = T2;
					else if (t4.ac == t) t4.ac = T2;
				}

				//}
			}
		}


	}


	return(1);
}

bool pointSortPredicate(const Shx& a, const Shx& b)
{
	if (a.r < b.r)
		return true;
	else if (a.r > b.r)
		return false;
	else if (a.c < b.c)
		return true;
	else
		return false;
};

bool pointComparisonPredicate(const Shx& a, const Shx& b)
{
	return a.r == b.r && a.c == b.c;
}

void FNavMeshManager::AddBlockingAABB(FVector3 aMin, FVector3 aMax)
{
	AABB aabb;
	FVector3 aDimensions = aMax - aMin;

	aMin = aMin - (aDimensions.Normalized() * 0.2f);
	aMax = aMax + (aDimensions.Normalized() * 0.2f);
	
	aabb.myMin = aMin;
	aabb.myMax = aMax;
	myAABBList.push_back(aabb);

	aMin.y = 0;
	aMax.y = 0;
	// add 4 corner points
	myPointList.push_back(aMin);
	myPointList.push_back(aMax);
	myPointList.push_back(FVector3(aMin.x, 0, aMax.z));
	myPointList.push_back(FVector3(aMax.x, 0, aMin.z));
}

void FNavMeshManager::RemoveAllBlockingAABB()
{
	myAABBList.clear();
	myPointList.clear();
}

void FNavMeshManager::GenerateMesh(FVector3 aMin, FVector3 aMax)
{
	Shx pt;
	srand(1);

	pts.clear();
	triads.clear();
	
	int v = 0;
	for (; v< myPointList.size(); v++) {
		pt.id = v;
		pt.r = myPointList[v].x;
		pt.c = myPointList[v].z;

		pts.push_back(pt);
	}

	pt.id = v++;
	pt.r = aMin.x;
	pt.c = aMin.z;
	pts.push_back(pt);
	pt.id = v++;
	pt.r = aMax.x;
	pt.c = aMax.z;
	pts.push_back(pt);

	pt.id = v++;
	pt.r = aMax.x;
	pt.c = aMin.z;
	pts.push_back(pt);
	pt.id = v++;
	pt.r = aMin.x;
	pt.c = aMax.z;
	pts.push_back(pt);

	hull = pts;//swap these, hull is the pts that were used.. pts is the original point list
	std::sort(pts.begin(), pts.end(), pointSortPredicate);
	std::vector<Shx>::iterator newEnd = std::unique(pts.begin(), pts.end(), pointComparisonPredicate);
	pts.resize(newEnd - pts.begin());

	s_hull_pro(pts, triads);

	// correct windings of triads
	for (int i = 0; i < triads.size(); i++)
	{
		FVector3 pa = FVector3(hull[triads[i].a].r, 0, hull[triads[i].a].c);
		FVector3 pb = FVector3(hull[triads[i].b].r, 0, hull[triads[i].b].c);
		FVector3 pc = FVector3(hull[triads[i].c].r, 0, hull[triads[i].c].c);
		FVector3 cross = (pb - pa).Cross((pc - pa));
		if (cross.y < 0.0f)
		{
			int newC = triads[i].a;
			triads[i].a = triads[i].c;
			triads[i].c = newC;

			int newBC = triads[i].ab;
			triads[i].ab = triads[i].bc;
			triads[i].bc = newBC;
		}
	}

	// set up blocked triangles
	myBlockedTriangleList.clear();
	myBlockedTriangleList.resize(triads.size(), false);
	int triIdx = 0;
	for (Triad& tri : triads)
	{
		FVector3 pa = FVector3(hull[tri.a].r, 0, hull[tri.a].c);
		FVector3 pb = FVector3(hull[tri.b].r, 0, hull[tri.b].c);
		FVector3 pc = FVector3(hull[tri.c].r, 0, hull[tri.c].c);

		for (AABB& aabb : myAABBList)
		{
			bool isInside = true;
			isInside &= aabb.IsInside(pa);
			isInside &= aabb.IsInside(pb);
			isInside &= aabb.IsInside(pc);

			if (!myBlockedTriangleList[triIdx])
				myBlockedTriangleList[triIdx] = isInside; // could check individually and early out after a point is inside
		}

		triIdx++;
	}
}

FNavMeshManager* FNavMeshManager::myInstance = nullptr;
FNavMeshManager * FNavMeshManager::GetInstance()
{
	if (!myInstance)
		myInstance = new FNavMeshManager();
	return myInstance;
}

FNavMeshManager::FNavMeshManager()
{
	// Test code to check navmesh generation
	/*
	Shx pt;
	srand(1);

	//for(int v=0; v<20000; v++){
	for (int v = 0; v<100; v++) {
		pt.id = v;
		pt.r = (float)(rand() % 20);
		pt.c = (float)(rand() % 20);

		pts.push_back(pt);
	}

	hull = pts;//swap these, hull is the pts that were used.. pts is the original point list
	std::sort(pts.begin(), pts.end(), pointSortPredicate);
	std::vector<Shx>::iterator newEnd = std::unique(pts.begin(), pts.end(), pointComparisonPredicate);
	pts.resize(newEnd - pts.begin());
	
	s_hull_pro(pts, triads);

	// correct windings of triads
	for (int i = 0; i < triads.size(); i++)
	{
		FVector3 pa = FVector3(hull[triads[i].a].r, 0, hull[triads[i].a].c);
		FVector3 pb = FVector3(hull[triads[i].b].r, 0, hull[triads[i].b].c);
		FVector3 pc = FVector3(hull[triads[i].c].r, 0, hull[triads[i].c].c);
		FVector3 cross = (pb - pa).Cross((pc - pa));
		if (cross.y < 0.0f)
		{
			int newC = triads[i].a;
			triads[i].a = triads[i].c;
			triads[i].c = newC;

			int newBC = triads[i].ab;
			triads[i].ab = triads[i].bc;
			triads[i].bc = newBC;
		}
	}
	*/
}


FNavMeshManager::~FNavMeshManager()
{
}

void FNavMeshManager::DebugDraw(FDebugDrawer * aDebugDrawer)
{
	FPROFILE_FUNCTION("NavMeshDebugDraw");

	for (int i = 0; i < pts.size(); i++)
	{
		FVector3 pos = FVector3(pts[i].r, 0.1f, pts[i].c);
		float size = 0.1f;
		aDebugDrawer->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(1, 1, 0));
	}

	for (int i = 0; i < triads.size(); i++)
	{
		Shx& pointA = hull[triads[i].a];
		Shx& pointB = hull[triads[i].b];
		Shx& pointC = hull[triads[i].c];
		FVector3 vPointA = FVector3(pointA.r, 0, pointA.c);
		FVector3 vPointB = FVector3(pointB.r, 0, pointB.c);
		FVector3 vPointC = FVector3(pointC.r, 0, pointC.c);
		FVector3 vCenter = (vPointA + vPointB + vPointC) / 3.0f;
		float shrinkFact = 0.0f;
		vPointA -= (vPointA - vCenter) * shrinkFact;
		vPointB -= (vPointB - vCenter) * shrinkFact;
		vPointC -= (vPointC - vCenter) * shrinkFact;

		FVector3 color = FVector3(0, 0.3f + (i % 20) / 20.0f, 0);

		// this was for highlighting the triangle winding
		int highlightTri = 9999; // 4,0,2,8
		if (IsBlocked(i))
			color = FVector3(0.3f + ((i % 20) / 20.0f), 0, 0);
		if (highlightTri == i)
			color = FVector3(0, 0, (i % 20) / 20.0f);
		aDebugDrawer->DrawTriangle(vPointA, vPointB, vPointC, color);

		if (i == highlightTri)
		{
			float size = 0.3f;
			aDebugDrawer->DrawTriangle(vPointA + FVector3(-size, 0, -size), vPointA + FVector3(-size, 0, size), vPointA + FVector3(size, 0, size), FVector3(1, 0, 0));
			aDebugDrawer->DrawTriangle(vPointB + FVector3(-size, 0, -size), vPointB + FVector3(-size, 0, size), vPointB + FVector3(size, 0, size), FVector3(0, 1, 0));
			aDebugDrawer->DrawTriangle(vPointC + FVector3(-size, 0, -size), vPointC + FVector3(-size, 0, size), vPointC + FVector3(size, 0, size), FVector3(0, 0, 1));
		}

	}

	for (int i = 1; i < myPathList.myPathPoints.size(); i++)
	{
		float shade = 0.5f + (i * (0.5f / myPathList.myPathPoints.size()));
		FVector3 from = FVector3(myPathList.myPathPoints[i - 1].x, 0.1f, myPathList.myPathPoints[i - 1].z);
		FVector3 to = FVector3(myPathList.myPathPoints[i].x, 0.1f, myPathList.myPathPoints[i].z);
		aDebugDrawer->drawLine(from, to, FVector3(shade, shade, shade));
	}

	for (int i = 0; i < myPathList.myFunnel.size(); i++)
	{
		float shade = 0.5f + (i * (0.5f / myPathList.myFunnel.size()));
		float size = 0.1f;
		FVector3 pos = myPathList.myFunnel[i].aLeft;
		pos.y = 0.2f;
		aDebugDrawer->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(1, 0.5f, 0.5f));
		pos = myPathList.myFunnel[i].aRight;
		pos.y = 0.15f;
		size = 0.15f;
		aDebugDrawer->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(0, 1, 0));
	}

}

bool FNavMeshManager::IsInsideTriangle(FVector3 aPos, FVector3 aV1, FVector3 aV2, FVector3 aV3)
{
	float p0x = aV1.x;
	float p0y = aV1.z;
	float p1x = aV2.x;
	float p1y = aV2.z;
	float p2x = aV3.x;
	float p2y = aV3.z;
	float px = aPos.x;
	float py = aPos.z;

	float Area = 0.5 *(-p1y*p2x + p0y*(-p1x + p2x) + p0x*(p1y - p2y) + p1x*p2y);
	float s = 1.0f / (2.0f * Area)*(p0y*p2x - p0x*p2y + (p2y - p0y)*px + (p0x - p2x)*py);
	float t = 1.0f / (2.0f * Area)*(p0x*p1y - p0y*p1x + (p0y - p1y)*px + (p1x - p0x)*py);
	if (s > 0 && t > 0 && 1 - s - t > 0)
		return true;

	return false;
}

FVector3 FNavMeshManager::GetCenter(int aTriangleIdx)
{
	Shx& pointA = hull[triads[aTriangleIdx].a];
	Shx& pointB = hull[triads[aTriangleIdx].b];
	Shx& pointC = hull[triads[aTriangleIdx].c];
	FVector3 vPointA = FVector3(pointA.r, 0, pointA.c);
	FVector3 vPointB = FVector3(pointB.r, 0, pointB.c);
	FVector3 vPointC = FVector3(pointC.r, 0, pointC.c);
	return (vPointA + vPointB + vPointC) / 3.0f;
}
int FNavMeshManager::GetTriangleForPoint(FVector3 aPos)
{
	for (int i = 0; i < triads.size(); i++)
	{
		Shx& pointA = hull[triads[i].a];
		Shx& pointB = hull[triads[i].b];
		Shx& pointC = hull[triads[i].c];
		FVector3 vPointA = FVector3(pointA.r, 0, pointA.c);
		FVector3 vPointB = FVector3(pointB.r, 0, pointB.c);
		FVector3 vPointC = FVector3(pointC.r, 0, pointC.c);
		if (IsInsideTriangle(aPos, vPointA, vPointB, vPointC))
			return i;
	}

	return -1;
}

float FNavMeshManager::GetDistanceTriangleToPoint(int aTriangleIdx, FVector3 aPos)
{
	std::vector<Shx> points;
	Shx& pointA = hull[triads[aTriangleIdx].a];
	Shx& pointB = hull[triads[aTriangleIdx].b];
	Shx& pointC = hull[triads[aTriangleIdx].c];
	points.push_back(pointA); // performance hit now, could be expanded to convex poly's
	points.push_back(pointB);
	points.push_back(pointC);

	float dist = 100000000.0f;
	FVector3 posOnLine;

	for (int i = 0; i < points.size(); i++)
	{
		int prevIdx = i == 0 ? points.size() - 1 : i - 1;

		float x1 = points[prevIdx].r;
		float y1 = points[prevIdx].c;
		float x2 = points[i].r;
		float y2 = points[i].c;
		float A = aPos.x - x1;
		float B = aPos.z - y1;
		float C = x2 - x1;
		float D = y2 - y1;

		float dot = A * C + B * D;
		float len_sq = C * C + D * D;
		float param = dot / len_sq;
		float xx, yy;

		if (param < 0)
		{
			xx = x1;
			yy = y1;
		}
		else if (param > 1)
		{
			xx = x2;
			yy = y2;
		}
		else
		{
			xx = x1 + param * C;
			yy = y1 + param * D;
		}

		float newDist = abs(A * D - C * B) / sqrt(C * C + D * D);
		dist = newDist < dist ? newDist : dist;
		//FVector3 posOnLine = FVector3(xx, 0, yy); // align on y
	}
	
	return dist;
	//return (GetCenter(aTriangleIdx) - aPos).Length();
}

FVector3 FNavMeshManager::GetClosestPointOnTriToPointAllowedTri(int aTriangleIdx, FVector3 aPos, int aFromTriangle)
{
	std::vector<Shx> points;
	Shx& pointA = hull[triads[aTriangleIdx].a];
	Shx& pointB = hull[triads[aTriangleIdx].b];
	Shx& pointC = hull[triads[aTriangleIdx].c];
	points.push_back(pointA); // performance hit now, could be expanded to convex poly's
	points.push_back(pointB);
	points.push_back(pointC);

	float dist = 100000000.0f;
	FVector3 posOnLine;

	for (int i = 0; i < points.size(); i++)
	{
		int prevIdx = i == 0 ? points.size() - 1 : i - 1;

		// hack: check if the edge is connected to us, otherwise skip - this is to prevent skipping over a triangle to get to an edge
		if (i == 0)
		{
			if (triads[aTriangleIdx].ac != aFromTriangle)
				continue;
		}

		if (i == 1)
		{
			if (triads[aTriangleIdx].ab != aFromTriangle)
				continue;
		}

		if (i == 2)
		{
			if (triads[aTriangleIdx].bc != aFromTriangle)
				continue;
		}

		float x1 = points[prevIdx].r;
		float y1 = points[prevIdx].c;
		float x2 = points[i].r;
		float y2 = points[i].c;
		float A = aPos.x - x1;
		float B = aPos.z - y1;
		float C = x2 - x1;
		float D = y2 - y1;

		float dot = A * C + B * D;
		float len_sq = C * C + D * D;
		float param = dot / len_sq;
		float xx, yy;

		if (param < 0)
		{
			xx = x1;
			yy = y1;
		}
		else if (param > 1)
		{
			xx = x2;
			yy = y2;
		}
		else
		{
			xx = x1 + param * C;
			yy = y1 + param * D;
		}

		float newDist = abs(A * D - C * B) / sqrt(C * C + D * D);
		FVector3 newPosOnLine = FVector3(xx, 0, yy);
		float newDistAgain = (newPosOnLine - aPos).Length();
		if (newDistAgain < dist)
		{
			dist = newDistAgain;
			posOnLine = FVector3(xx, 0, yy); // align on y
		}
	}

	return posOnLine;
}

FVector3 FNavMeshManager::GetClosestPointOnTriToPoint(int aTriangleIdx, FVector3 aPos)
{
	std::vector<Shx> points;
	Shx& pointA = hull[triads[aTriangleIdx].a];
	Shx& pointB = hull[triads[aTriangleIdx].b];
	Shx& pointC = hull[triads[aTriangleIdx].c];
	points.push_back(pointA); // performance hit now, could be expanded to convex poly's
	points.push_back(pointB);
	points.push_back(pointC);

	float dist = 100000000.0f;
	FVector3 posOnLine;

	for (int i = 0; i < points.size(); i++)
	{
		int prevIdx = i == 0 ? points.size() - 1 : i - 1;

		float x1 = points[prevIdx].r;
		float y1 = points[prevIdx].c;
		float x2 = points[i].r;
		float y2 = points[i].c;
		float A = aPos.x - x1;
		float B = aPos.z - y1;
		float C = x2 - x1;
		float D = y2 - y1;

		float dot = A * C + B * D;
		float len_sq = C * C + D * D;
		float param = dot / len_sq;
		float xx, yy;

		if (param < 0)
		{
			xx = x1;
			yy = y1;
		}
		else if (param > 1)
		{
			xx = x2;
			yy = y2;
		}
		else
		{
			xx = x1 + param * C;
			yy = y1 + param * D;
		}

		float newDist = abs(A * D - C * B) / sqrt(C * C + D * D);
		FVector3 newPosOnLine = FVector3(xx, 0, yy);
		float newDistAgain = (newPosOnLine - aPos).Length();
		if (newDistAgain < dist)
		{
			dist = newDistAgain;
			posOnLine = FVector3(xx, 0, yy); // align on y
		}
	}

	return posOnLine;
}

std::vector<FVector3> FNavMeshManager::FindPath(FVector3 aStart, FVector3 anEnd)
{
	const float infDist = 1000000.0f;

	aStart.y = 0; anEnd.y = 0;

	myFunnel.clear();

	std::vector<Triad> openList = triads;
	std::vector<int> path;
	std::vector<int> prevNodes;
	std::vector<FVector3> result;
	std::vector<bool> visitedList;
	std::vector<float> distances;
	visitedList.resize(openList.size(), false);
	distances.resize(openList.size(), infDist);
	prevNodes.resize(openList.size(), -1);
	float curDist = infDist;
	
	int startTriangle = GetTriangleForPoint(aStart);
	int endTriangle = GetTriangleForPoint(anEnd);
	
	result.push_back(aStart);

	FunnelNode startNode;
	startNode.aLeft = aStart;
	startNode.aRight = aStart;

	if (startTriangle < 0 || endTriangle < 0)
		return std::vector<FVector3>();

	// this is depth first gready, todo: replace with proper dijkstra	
	int curTri = startTriangle;
	distances[startTriangle] = 0.0f;

	// DIJKSTRA
	//*
	bool shouldContinue = true;
	while (shouldContinue)
	{
		shouldContinue = false;

		// find node with smallest distance to target
		float smallestDist = infDist;
		for (int i = 0; i < triads.size(); i++)
		{
			if (!visitedList[i] && distances[i] < smallestDist)
			{
				curTri = i;
				curDist = distances[i];
				smallestDist = distances[i];
				shouldContinue = true;
			}
		}

		// done condition
		if (curTri == endTriangle)
		{
			shouldContinue = false;
		}

		visitedList[curTri] = true;

		// evaluate children and pick closest to target
		float dist1 = (triads[curTri].ab < 0 || visitedList[triads[curTri].ab] || myBlockedTriangleList[triads[curTri].ab]) ? infDist : GetDistanceTriangleToPoint(triads[curTri].ab, anEnd); // @todo: cache weightmap before
		float dist2 = (triads[curTri].ac < 0 || visitedList[triads[curTri].ac] || myBlockedTriangleList[triads[curTri].ac]) ? infDist : GetDistanceTriangleToPoint(triads[curTri].ac, anEnd);
		float dist3 = (triads[curTri].bc < 0 || visitedList[triads[curTri].bc] || myBlockedTriangleList[triads[curTri].bc]) ? infDist : GetDistanceTriangleToPoint(triads[curTri].bc, anEnd);

		if (dist1 == infDist && dist2 == infDist && dist3 == infDist)
		{
			if (curTri == startTriangle)
			{
				OutputDebugStringA("Path could not be computed\n"); //path impossible
				break;
			}
			continue;
		}

		// check all 3 distances and pick closest, set prev reference for backtrace
		float newDist = curDist + dist1;
		if(triads[curTri].ab >= 0 && newDist < distances[triads[curTri].ab])
		{
			distances[triads[curTri].ab] = newDist;
			prevNodes[triads[curTri].ab] = curTri;
		}

		newDist = curDist + dist2;
		if(triads[curTri].ac >= 0 && newDist < distances[triads[curTri].ac])
		{
			distances[triads[curTri].ac] = newDist;
			prevNodes[triads[curTri].ac] = curTri;
		}

		newDist = curDist + dist3;
		if (triads[curTri].bc >= 0 && newDist < distances[triads[curTri].bc])
		{
			distances[triads[curTri].bc] = newDist;
			prevNodes[triads[curTri].bc] = curTri;
		}

	}

	// backtrace
	path.clear();
	int tri = endTriangle;
	while (tri != startTriangle)
	{
		if (tri == -1)
		{
			OutputDebugStringA("PrevNode not set, abort backtrace\n");
			return std::vector<FVector3>();
			//break;
		}

		path.push_back(tri);
		tri = prevNodes[tri];
	}

	// make nodes out of the triangles (path is stored backwards)
	// currently making a funnel for smoothing and a raw path (in case funnel algorithm fails we can return raw nodes)
	FVector3 curPos2 = aStart;
	int curTri2 = startTriangle;
	for (int i = path.size() - 1; i >= 0; i--)
	{
		curPos2 = GetClosestPointOnTriToPointAllowedTri(path[i], curPos2, curTri2);  // this actually is not good - can skip over edges - get closest point to any edge? connected to me?

		result.push_back(curPos2);

		FunnelNode funnelNode;
		if (curTri2 == triads[path[i]].ab)
		{
			funnelNode.aRight = FVector3(hull[triads[path[i]].a].r, 0, hull[triads[path[i]].a].c);
			funnelNode.aLeft = FVector3(hull[triads[path[i]].b].r, 0, hull[triads[path[i]].b].c);
		}
		if (curTri2 == triads[path[i]].bc)
		{
			funnelNode.aRight = FVector3(hull[triads[path[i]].b].r, 0, hull[triads[path[i]].b].c);
			funnelNode.aLeft = FVector3(hull[triads[path[i]].c].r, 0, hull[triads[path[i]].c].c);
		}
		if (curTri2 == triads[path[i]].ac)
		{
			funnelNode.aRight = FVector3(hull[triads[path[i]].c].r, 0, hull[triads[path[i]].c].c);
			funnelNode.aLeft = FVector3(hull[triads[path[i]].a].r, 0, hull[triads[path[i]].a].c);
		}

		curTri2 = path[i];

		myFunnel.push_back(funnelNode);
	}
	result.push_back(anEnd);

	// funnel endnode
	FunnelNode endNode;
	endNode.aRight = anEnd;
	endNode.aLeft = anEnd;
	myFunnel.push_back(endNode);

	//optimize path + smooth
	// resolve funnel
	float currentFunnelDot = -10.0f;
	int i = 0;
	std::vector<FVector3> funnelResolvedPath;
	funnelResolvedPath.push_back(aStart);
	FVector3 curPos = aStart;
	while (i < myFunnel.size())
	{
		FVector3 left = myFunnel[i].aLeft;
		FVector3 right = myFunnel[i].aRight;
		currentFunnelDot = (left-curPos).Normalized().Dot((right - curPos).Normalized());
		int oldi = i;
		int iLeft = i + 1;
		int iRight = i + 1;
		for (int j = i + 1; j < myFunnel.size(); j++)
		{
			float x1 = curPos.x;
			float y1 = curPos.z;

			// try narrow funnel by moving right node
			{
				float newDot = (left - curPos).Normalized().Dot((myFunnel[j].aRight - curPos).Normalized());

				// if dot is bigger, funnel got smaller - use new node
				if (newDot > currentFunnelDot)
				{
					right = myFunnel[j].aRight;
					iRight = j + 1;
					currentFunnelDot = newDot;
				}
				
				// check if lines cross each other, if so set new pos and restart
				FVector3 crossTest = (left - curPos).Cross(myFunnel[j].aRight - curPos);
				if (crossTest.y < 0)
				{
					curPos = left;
					i = iLeft;
					funnelResolvedPath.push_back(curPos);
					break;
				}
			}

			// try narrowing by moving the left node
			{
				float newDot = (myFunnel[j].aLeft - curPos).Normalized().Dot((right - curPos).Normalized());

				// if dot is bigger, funnel got smaller - use new node
				if (newDot > currentFunnelDot)
				{
					iLeft = j + 1;
					left = myFunnel[j].aLeft;
					currentFunnelDot = newDot;
				}

				// check again if lines cross each other, if so set new pos and restart
				FVector3 crossTest = (myFunnel[j].aLeft - curPos).Cross(right - curPos);
				if (crossTest.y < 0)
				{
					curPos = right;
					i = iRight;
					funnelResolvedPath.push_back(curPos);
					break;
				}
			}
		}

		if (oldi == i)
			i++; // nothing changed, all nodes are inside funnel?
	}
	funnelResolvedPath.push_back(anEnd);

	myPathList.myFunnel = myFunnel;
	myPathList.myPathPoints = funnelResolvedPath;
	return funnelResolvedPath; //todo: fix funnel algorithm
	//return result;
}
