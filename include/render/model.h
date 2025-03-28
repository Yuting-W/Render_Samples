#ifndef MODEL_H
#define MODEL_H
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render/shader_s.h"
#include "render/mesh.h"


class Model 
{
	public:
		Model(const char* path, bool set_flip_vertically = 0);
		void Draw(Shader& shader, Material* draw_material = nullptr, bool disable_depth_test = 0);


	private:
		std::vector<Mesh> meshes;
		std::map<std::string, Texture*> loaded_textures;
		std::map<unsigned int, Material*> loaded_materials;
		std::string directory;
		FileFormat format;
		void loadModel(std::string path);

		void processNode(aiNode* node, const aiScene* scene);

		Mesh processMesh(aiMesh* mesh, const aiScene* scene);

		std::vector<Texture*> loadMaterialTextures(aiMaterial* material, aiTextureType type, TextureType texture_type, const std::string& material_name);
		
		void print_loaded_textures(const std::map<std::string, Texture*>& loaded_textures);
	
};

#endif // !1


