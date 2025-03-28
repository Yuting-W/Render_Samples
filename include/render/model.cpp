

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <texture/stb_image.h>


#include "render/model.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
//------------load 2D texture---------------------
unsigned int TextureFromFile(const char* path, const std::string& directory, int& nrComponents)
{
	std::string filename = std::string(path);
	if (directory != "")
		filename = directory + '/' + filename;
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		else
			std::cout << "Texture format is error. \n";

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

Model::Model(const char* path, bool set_flip_vertically)
{
	stbi_set_flip_vertically_on_load(set_flip_vertically);

	loadModel(path);
	print_loaded_textures(loaded_textures);

}
void Model::Draw(Shader& shader, Material* draw_material, bool disable_depth_test)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Draw(shader, draw_material, disable_depth_test);
	}
}

void Model::loadModel(std::string path)
{
	//import scene
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);  //melloc in readfile!! 
	//aiProcess_Triangulate : 如果模型不是（全部）由三角形组成，它需要将模型所有的图元形状变换为三角形
	//aiProcess_FlipUVs : 翻转y轴的纹理坐标， opengl大多数图像是反的
	//aiProcess_GenNormals: 如果模型不包含法向量的话，就为每个顶点创建法线。
	//aiProcess_SplitLargeMeshes: 将比较大的网格分割成更小的子网格，如果你的渲染有最大顶点数限制，只能渲染较小的网格，那么它会非常有用。
	//aiProcess_OptimizeMeshes: aiProcess_OptimizeMeshes
//check scene
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	//
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	//process mesh 
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]]; // nod->meshes[i] is the index of mesh in scene->mMeshes
		meshes.push_back(processMesh(mesh, scene));
	}
	//after we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}

}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	//process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;

		// normals
		if (mesh->HasNormals())
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
		}
		if (mesh->mTextureCoords[0])  // assimp allows multiple texture coordinates in a mesh, we only care about the first set of texture coordinates
		{
			vector.x = mesh->mTextureCoords[0][i].x;
			vector.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = glm::vec2(vector.x, vector.y);

			//// tangent
			//vector.x = mesh->mTangents[i].x;
			//vector.y = mesh->mTangents[i].y;
			//vector.z = mesh->mTangents[i].z;
			//vertex.Tangent = vector;
			//// bitangent
			//vector.x = mesh->mBitangents[i].x;
			//vector.y = mesh->mBitangents[i].y;
			//vector.z = mesh->mBitangents[i].z;
			//vertex.Bitangent = vector;


		}
		else
		{
			vertex.TexCoords = glm::vec2(0, 0);
		}
		vertices.push_back(vertex);
	}

	//process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// Get corresponding material from mesh
	aiMaterial* ai_material = scene->mMaterials[mesh->mMaterialIndex];
	// Material name
	std::string material_name = "mat_";
	if (std::string(ai_material->GetName().C_Str()) == "") {
		//std::ostringstream ss;
		//ss << std::setw(3) << std::setfill('0') << this->loaded_materials.size();
		//material_name += ss.str();
	}
	else {
		material_name += std::string(ai_material->GetName().C_Str());
	}

	Material* material = nullptr;

	// If material not loaded
	if (loaded_materials.find(mesh->mMaterialIndex) == loaded_materials.end()) {
		// 1. albedo maps
		std::vector<Texture*> albedoMaps = loadMaterialTextures(ai_material, aiTextureType_BASE_COLOR, TexAlbedo, material_name);
		if (albedoMaps.size() == 0)
		{
			albedoMaps = loadMaterialTextures(ai_material, aiTextureType_DIFFUSE, TexAlbedo, material_name);
		}
		// 2. normal maps
		std::vector<Texture*> normalMaps = loadMaterialTextures(ai_material, aiTextureType_NORMALS, TexNormal, material_name);
		// 3. metalness maps
		std::vector<Texture*> metalnessMaps = loadMaterialTextures(ai_material, aiTextureType_METALNESS, TexMetalness, material_name);
		// 4. roughness maps
		std::vector<Texture*> roughnessMaps = loadMaterialTextures(ai_material, aiTextureType_DIFFUSE_ROUGHNESS, TexRoughness, material_name);
		// 5. emission maps
		std::vector<Texture*> emissionMaps = loadMaterialTextures(ai_material, aiTextureType_EMISSIVE, TexEmission, material_name);
		// 6. ambient occlusion maps
		std::vector<Texture*> ambientOcclusionMaps = loadMaterialTextures(ai_material, aiTextureType_LIGHTMAP, TexAmbientOcclusion, material_name);
		// 7. specular maps
		std::vector<Texture*> specularMaps = loadMaterialTextures(ai_material, aiTextureType_SPECULAR, TexSpecular, material_name);

		// Create material of the mesh
		material = new Material(material_name);
		material->format = this->format;
		if (albedoMaps.size() != 0) {
			material->textures[TexAlbedo] = albedoMaps[0];
		}
		if (normalMaps.size() != 0) {
			material->textures[TexNormal] = normalMaps[0];
		}
		if (metalnessMaps.size() != 0) {
			material->textures[TexMetalness] = metalnessMaps[0];
		}
		if (roughnessMaps.size() != 0) {
			material->textures[TexRoughness] = roughnessMaps[0];
		}
		if (emissionMaps.size() != 0) {
			material->textures[TexEmission] = emissionMaps[0];
		}
		if (ambientOcclusionMaps.size() != 0) {
			material->textures[TexAmbientOcclusion] = ambientOcclusionMaps[0];
		}
		if (specularMaps.size() != 0) {
			material->textures[TexSpecular] = specularMaps[0];
		}
		loaded_materials[mesh->mMaterialIndex] = material;
	}
	else {
		material = loaded_materials[mesh->mMaterialIndex];
	}

	return Mesh(vertices, indices, material);
}

std::vector<Texture*> Model::loadMaterialTextures(aiMaterial* material, aiTextureType type, TextureType texture_type, const std::string& material_name)
{
	std::vector<Texture*> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		if (loaded_textures.find(std::string(str.C_Str())) != loaded_textures.end())
		{
			Texture* texture = loaded_textures[std::string(str.C_Str())];
			texture->types.insert(texture_type);
			textures.push_back(texture);
		}
		else {
			Texture* texture = new Texture("tex_" + material_name.substr(4));
			texture->id = TextureFromFile(str.C_Str(), directory, texture->num_channels);
			texture->types.insert(texture_type);
			texture->path = str.C_Str();
			textures.push_back(texture);
			loaded_textures[std::string(str.C_Str())] = texture;
		}


	}
	return textures;
}


void Model::print_loaded_textures(const std::map<std::string, Texture*>& loaded_textures) {
	for (auto it = loaded_textures.begin(); it != loaded_textures.end(); it++) {
		Texture* texture = it->second;
		std::cout << "Texture loaded at path: " << texture->path << ", with " << texture->num_channels << " channels, of types: ";
		for (auto it = texture->types.begin(); it != texture->types.end(); it++) {
			std::cout << Texture::get_long_name_of_texture_type(*it) << ", ";
		}
		std::cout << std::endl;
	}
}
