//
// g++ loader_example.cc
//
#define TINYOBJLOADER_IMPLEMENTATION
#include "FObjLoader.h"
#include "FPrimitiveGeometry.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
#include <mmsystem.h>
#ifdef __cplusplus
}
#endif
#pragma comment(lib, "winmm.lib")
#else
#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#else
#include <ctime>
#endif
#endif

#include "FUtilities.h"


class timerutil {
public:
#ifdef _WIN32
	typedef DWORD time_t;

	timerutil() { ::timeBeginPeriod(1); }
	~timerutil() { ::timeEndPeriod(1); }

	void start() { t_[0] = ::timeGetTime(); }
	void end() { t_[1] = ::timeGetTime(); }

	time_t sec() { return (time_t)((t_[1] - t_[0]) / 1000); }
	time_t msec() { return (time_t)((t_[1] - t_[0])); }
	time_t usec() { return (time_t)((t_[1] - t_[0]) * 1000); }
	time_t current() { return ::timeGetTime(); }

#else
#if defined(__unix__) || defined(__APPLE__)
	typedef unsigned long int time_t;

	void start() { gettimeofday(tv + 0, &tz); }
	void end() { gettimeofday(tv + 1, &tz); }

	time_t sec() { return static_cast<time_t>(tv[1].tv_sec - tv[0].tv_sec); }
	time_t msec() {
		return this->sec() * 1000 +
			static_cast<time_t>((tv[1].tv_usec - tv[0].tv_usec) / 1000);
	}
	time_t usec() {
		return this->sec() * 1000000 +
			static_cast<time_t>(tv[1].tv_usec - tv[0].tv_usec);
	}
	time_t current() {
		struct timeval t;
		gettimeofday(&t, NULL);
		return static_cast<time_t>(t.tv_sec * 1000 + t.tv_usec);
	}

#else  // C timer
	// using namespace std;
	typedef clock_t time_t;

	void start() { t_[0] = clock(); }
	void end() { t_[1] = clock(); }

	time_t sec() { return (time_t)((t_[1] - t_[0]) / CLOCKS_PER_SEC); }
	time_t msec() { return (time_t)((t_[1] - t_[0]) * 1000 / CLOCKS_PER_SEC); }
	time_t usec() { return (time_t)((t_[1] - t_[0]) * 1000000 / CLOCKS_PER_SEC); }
	time_t current() { return (time_t)clock(); }

#endif
#endif

private:
#ifdef _WIN32
	DWORD t_[2];
#else
#if defined(__unix__) || defined(__APPLE__)
	struct timeval tv[2];
	struct timezone tz;
#else
	time_t t_[2];
#endif
#endif
};

void FObjLoader::PrintInfo(const tinyobj::attrib_t& attrib,	const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials) 
{
	FUtilities::FLog("# of vertices    : %i\n", (attrib.vertices.size() / 3));
	FUtilities::FLog("# of normals    : %i\n", (attrib.normals.size() / 3));
	FUtilities::FLog("# of texcoords    : %i\n", (attrib.texcoords.size() / 2));
	FUtilities::FLog("# of shapes    : %i\n", shapes.size());
	FUtilities::FLog("# of materials    : %i\n", materials.size());

	for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
		FUtilities::FLog("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
			static_cast<const double>(attrib.vertices[3 * v + 0]),
			static_cast<const double>(attrib.vertices[3 * v + 1]),
			static_cast<const double>(attrib.vertices[3 * v + 2]));
	}

	for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
		FUtilities::FLog("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
			static_cast<const double>(attrib.normals[3 * v + 0]),
			static_cast<const double>(attrib.normals[3 * v + 1]),
			static_cast<const double>(attrib.normals[3 * v + 2]));
	}

	for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
		FUtilities::FLog("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
			static_cast<const double>(attrib.texcoords[2 * v + 0]),
			static_cast<const double>(attrib.texcoords[2 * v + 1]));
	}

	// For each shape
	for (size_t i = 0; i < shapes.size(); i++) {
		FUtilities::FLog("shape[%ld].name = %s\n", static_cast<long>(i),
			shapes[i].name.c_str());
		FUtilities::FLog("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
			static_cast<unsigned long>(shapes[i].mesh.indices.size()));

		size_t index_offset = 0;

		assert(shapes[i].mesh.num_face_vertices.size() ==
			shapes[i].mesh.material_ids.size());

		FUtilities::FLog("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
			static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

		// For each face
		for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
			size_t fnum = shapes[i].mesh.num_face_vertices[f];

			FUtilities::FLog("  face[%ld].fnum = %ld\n", static_cast<long>(f),
				static_cast<unsigned long>(fnum));

			// For each vertex in the face
			for (size_t v = 0; v < fnum; v++) {
				tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
				printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
					static_cast<long>(v), idx.vertex_index, idx.normal_index,
					idx.texcoord_index);
			}

			FUtilities::FLog("  face[%ld].material_id = %d\n", static_cast<long>(f),
				shapes[i].mesh.material_ids[f]);

			index_offset += fnum;
		}

		FUtilities::FLog("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
			static_cast<unsigned long>(shapes[i].mesh.tags.size()));
		for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
			FUtilities::FLog("  tag[%ld] = %s ", static_cast<long>(t),
				shapes[i].mesh.tags[t].name.c_str());
			FUtilities::FLog(" ints: [");
			for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
				FUtilities::FLog("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
				if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
					FUtilities::FLog(", ");
				}
			}
			FUtilities::FLog("]");

			FUtilities::FLog(" floats: [");
			for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
				FUtilities::FLog("%f", static_cast<const double>(
					shapes[i].mesh.tags[t].floatValues[j]));
				if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
					FUtilities::FLog(", ");
				}
			}
			FUtilities::FLog("]");

			FUtilities::FLog(" strings: [");
			for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
				FUtilities::FLog("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
				if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
					FUtilities::FLog(", ");
				}
			}
			FUtilities::FLog("]");
			FUtilities::FLog("\n");
		}
	}

	for (size_t i = 0; i < materials.size(); i++) {
		FUtilities::FLog("material[%ld].name = %s\n", static_cast<long>(i),
			materials[i].name.c_str());
		FUtilities::FLog("  material.Ka = (%f, %f ,%f)\n",
			static_cast<const double>(materials[i].ambient[0]),
			static_cast<const double>(materials[i].ambient[1]),
			static_cast<const double>(materials[i].ambient[2]));
		FUtilities::FLog("  material.Kd = (%f, %f ,%f)\n",
			static_cast<const double>(materials[i].diffuse[0]),
			static_cast<const double>(materials[i].diffuse[1]),
			static_cast<const double>(materials[i].diffuse[2]));
		FUtilities::FLog("  material.Ks = (%f, %f ,%f)\n",
			static_cast<const double>(materials[i].specular[0]),
			static_cast<const double>(materials[i].specular[1]),
			static_cast<const double>(materials[i].specular[2]));
		FUtilities::FLog("  material.Tr = (%f, %f ,%f)\n",
			static_cast<const double>(materials[i].transmittance[0]),
			static_cast<const double>(materials[i].transmittance[1]),
			static_cast<const double>(materials[i].transmittance[2]));
		FUtilities::FLog("  material.Ke = (%f, %f ,%f)\n",
			static_cast<const double>(materials[i].emission[0]),
			static_cast<const double>(materials[i].emission[1]),
			static_cast<const double>(materials[i].emission[2]));
		FUtilities::FLog("  material.Ns = %f\n",
			static_cast<const double>(materials[i].shininess));
		FUtilities::FLog("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
		FUtilities::FLog("  material.dissolve = %f\n",
			static_cast<const double>(materials[i].dissolve));
		FUtilities::FLog("  material.illum = %d\n", materials[i].illum);
		FUtilities::FLog("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
		FUtilities::FLog("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
		FUtilities::FLog("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
		FUtilities::FLog("  material.map_Ns = %s\n",
			materials[i].specular_highlight_texname.c_str());
		FUtilities::FLog("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
		FUtilities::FLog("    bump_multiplier = %f\n", static_cast<const double>(materials[i].bump_texopt.bump_multiplier));
		FUtilities::FLog("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
		FUtilities::FLog("  material.disp = %s\n", materials[i].displacement_texname.c_str());
		FUtilities::FLog("  <<PBR>>\n");
		FUtilities::FLog("  material.Pr     = %f\n", static_cast<const double>(materials[i].roughness));
		FUtilities::FLog("  material.Pm     = %f\n", static_cast<const double>(materials[i].metallic));
		FUtilities::FLog("  material.Ps     = %f\n", static_cast<const double>(materials[i].sheen));
		FUtilities::FLog("  material.Pc     = %f\n", static_cast<const double>(materials[i].clearcoat_thickness));
		FUtilities::FLog("  material.Pcr    = %f\n", static_cast<const double>(materials[i].clearcoat_thickness));
		FUtilities::FLog("  material.aniso  = %f\n", static_cast<const double>(materials[i].anisotropy));
		FUtilities::FLog("  material.anisor = %f\n", static_cast<const double>(materials[i].anisotropy_rotation));
		FUtilities::FLog("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
		FUtilities::FLog("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
		FUtilities::FLog("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
		FUtilities::FLog("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
		FUtilities::FLog("  material.norm   = %s\n", materials[i].normal_texname.c_str());
		std::map<std::string, std::string>::const_iterator it(
			materials[i].unknown_parameter.begin());
		std::map<std::string, std::string>::const_iterator itEnd(
			materials[i].unknown_parameter.end());

		for (; it != itEnd; it++) {
			FUtilities::FLog("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
		}
		FUtilities::FLog("\n");
	}
}

bool FObjLoader::TestLoadObj(const char* filename, const char* basepath, bool triangulate) 
{
	FUtilities::FLog("Loading %s\n", filename);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	timerutil t;
	t.start();
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);
	t.end();
	FUtilities::FLog("Parsing time: %lu [msecs]\n", t.msec());

	if (!err.empty()) {
		FUtilities::FLog("%s\n", err.c_str());
	}

	if (!ret) {
		FUtilities::FLog("Failed to load/parse .obj.\n");
		return false;
	}

	//PrintInfo(attrib, shapes, materials);

	return true;
}


bool FObjLoader::LoadObj(const char* filename, FObjMesh& aMesh, const char* basepath, bool triangulate) 
{
	FUtilities::FLog("Loading %s\n", filename);

	tinyobj::attrib_t& attrib = aMesh.myAttributes;
	std::vector<tinyobj::shape_t>& shapes = aMesh.myShapes;
	std::vector<tinyobj::material_t> materials = aMesh.myMaterials;

	timerutil t;
	t.start();
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);
	t.end();
	FUtilities::FLog("Parsing time: %lu [msecs]\n", t.msec());

	if (!err.empty()) {
		FUtilities::FLog("%s\n", err.c_str());
	}

	if (!ret) {
		FUtilities::FLog("Failed to load/parse .obj.\n");
		return false;
	}

	// PrintInfo(attrib, shapes, materials);

	return true;
}

bool FObjLoader::TestStreamLoadObj() {
	FUtilities::FLog("tream Loading.\n");

	std::stringstream objStream;
	objStream << "mtllib cube.mtl\n"
		"\n"
		"v 0.000000 2.000000 2.000000\n"
		"v 0.000000 0.000000 2.000000\n"
		"v 2.000000 0.000000 2.000000\n"
		"v 2.000000 2.000000 2.000000\n"
		"v 0.000000 2.000000 0.000000\n"
		"v 0.000000 0.000000 0.000000\n"
		"v 2.000000 0.000000 0.000000\n"
		"v 2.000000 2.000000 0.000000\n"
		"# 8 vertices\n"
		"\n"
		"g front cube\n"
		"usemtl white\n"
		"f 1 2 3 4\n"
		"g back cube\n"
		"# expects white material\n"
		"f 8 7 6 5\n"
		"g right cube\n"
		"usemtl red\n"
		"f 4 3 7 8\n"
		"g top cube\n"
		"usemtl white\n"
		"f 5 1 4 8\n"
		"g left cube\n"
		"usemtl green\n"
		"f 5 6 2 1\n"
		"g bottom cube\n"
		"usemtl white\n"
		"f 2 6 7 3\n"
		"# 6 elements";

	std::string matStream(
		"newmtl white\n"
		"Ka 0 0 0\n"
		"Kd 1 1 1\n"
		"Ks 0 0 0\n"
		"\n"
		"newmtl red\n"
		"Ka 0 0 0\n"
		"Kd 1 0 0\n"
		"Ks 0 0 0\n"
		"\n"
		"newmtl green\n"
		"Ka 0 0 0\n"
		"Kd 0 1 0\n"
		"Ks 0 0 0\n"
		"\n"
		"newmtl blue\n"
		"Ka 0 0 0\n"
		"Kd 0 0 1\n"
		"Ks 0 0 0\n"
		"\n"
		"newmtl light\n"
		"Ka 20 20 20\n"
		"Kd 1 1 1\n"
		"Ks 0 0 0");

	using namespace tinyobj;
	class MaterialStringStreamReader : public MaterialReader {
	public:
		MaterialStringStreamReader(const std::string& matSStream)
			: m_matSStream(matSStream) {}
		virtual ~MaterialStringStreamReader() {}
		virtual bool operator()(const std::string& matId,
			std::vector<material_t>* materials,
			std::map<std::string, int>* matMap,
			std::string* err) {
			(void)matId;
			std::string warning;
			LoadMtl(matMap, materials, &m_matSStream, &warning);

			if (!warning.empty()) {
				if (err) {
					(*err) += warning;
				}
			}
			return true;
		}

	private:
		std::stringstream m_matSStream;
	};

	MaterialStringStreamReader matSSReader(matStream);
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &objStream,
		&matSSReader);

	if (!err.empty()) {
		FUtilities::FLog("%s\n", err.c_str());
	}

	if (!ret) {
		return false;
	}

	PrintInfo(attrib, shapes, materials);

	return true;
}

FObjLoader::FObjLoader()
{
	FUtilities::FLog("FOBJLoader triggered \n");
}


FObjLoader::~FObjLoader()
{
}