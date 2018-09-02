#include "F3DModel.h"
#include "FObjLoader.h"
#include <unordered_map>


void hash_combine(size_t &seed, size_t hash)
{
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

namespace std {
	template<> struct hash<F3DModel::FVertex> {
		size_t operator()(F3DModel::FVertex const& vertex) const {

			size_t seed = 0;
			seed += vertex.myDiffuseMatId;
			hash<float> hasher;
			hash_combine(seed, hasher(vertex.position.x));
			hash_combine(seed, hasher(vertex.position.y));
			hash_combine(seed, hasher(vertex.position.z));
			hash_combine(seed, hasher(vertex.normal.x));
			hash_combine(seed, hasher(vertex.normal.y));
			hash_combine(seed, hasher(vertex.normal.z));
			hash_combine(seed, hasher(vertex.uv.x));
			hash_combine(seed, hasher(vertex.uv.y));
			return seed;
		}
	};
}


bool F3DModel::Load(const char * aPath)
{
	FObjLoader objLoader;
	FObjLoader::FObjMesh m;
	objLoader.LoadObj(aPath, m, "models/", true);

	//

	std::unordered_map<FVertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : m.myShapes)
	{
		int idx2 = 0;
		int matId = 999;

		if (shape.mesh.material_ids.size() > 0)
			matId = shape.mesh.material_ids[0];

		for (const auto& index : shape.mesh.indices)
		{
			if ((idx2 % 3) == 0)
			{
				matId = shape.mesh.material_ids[idx2 / 3];
			}
			idx2++;

			FVertex vertex;

			vertex.position.x = m.myAttributes.vertices[3 * index.vertex_index + 0];
			vertex.position.y = m.myAttributes.vertices[3 * index.vertex_index + 1];
			vertex.position.z = m.myAttributes.vertices[3 * index.vertex_index + 2];

			if (m.myAttributes.normals.size() > 0)
			{
				vertex.normal.x = m.myAttributes.normals[3 * index.normal_index + 0];
				vertex.normal.y = m.myAttributes.normals[3 * index.normal_index + 1];
				vertex.normal.z = m.myAttributes.normals[3 * index.normal_index + 2];
			}

			if (m.myAttributes.texcoords.size() > 0)
			{
				vertex.uv.x = m.myAttributes.texcoords[2 * index.texcoord_index + 0];
				vertex.uv.y = 1.0f - m.myAttributes.texcoords[2 * index.texcoord_index + 1];
			}

			vertex.myDiffuseMatId = matId == -1 ? 0 : matId;
			if (matId >= 0 && matId < m.myMaterials.size())
				vertex.myNormalMatId = m.myMaterials[matId].bump_texname.empty() ? 99 : matId;

			if (matId >= 0 && matId < m.myMaterials.size())
				vertex.mySpecularMatId = m.myMaterials[matId].specular_texname.empty() ? 99 : matId;

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(myVertices.size());
				myVertices.push_back(vertex);
			}

			myIndices.push_back(uniqueVertices[vertex]);
		}
	}

	std::string name;
	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		myMaterials.push_back(FMaterial());

		const tinyobj::material_t& mat = m.myMaterials[i];

		if (!mat.diffuse_texname.empty())
		{
			name = mat.diffuse_texname.substr(mat.diffuse_texname.find('\\') + 1, mat.diffuse_texname.length());
			myMaterials[myMaterials.size() - 1].myDiffuseTexture = name;
		}

		if (!mat.bump_texname.empty())
		{
			name = mat.bump_texname.substr(mat.bump_texname.find('\\') + 1, mat.bump_texname.length());
			myMaterials[myMaterials.size() - 1].myNormalTexture = name;
		}

		if (!mat.specular_texname.empty())
		{
			name = mat.specular_texname.substr(mat.specular_texname.find('\\') + 1, mat.specular_texname.length());
			myMaterials[myMaterials.size() - 1].mySpecularTexture = name;
		}
	}

	return true;
}

F3DModel::F3DModel()
{
}


F3DModel::~F3DModel()
{
}
