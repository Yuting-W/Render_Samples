#ifndef  MESH_H
#define  MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render/shader_s.h"
#include <set>
#include <map>
#include <string>
#include <vector>

#define MAX_BONE_INFLUENCE 4

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;

	glm::vec3 Tangent; // ?ÇÐÏß¿Õ¼ä
	glm::vec3 Bitangent;

	int m_BoneIDs[MAX_BONE_INFLUENCE]; //bone indexes which will influence this vertex
	float m_Weights[MAX_BONE_INFLUENCE]; // weithts of each bone
};
enum TextureType {
	TexAlbedo, TexNormal, TexMetalness, TexRoughness, TexEmission, TexAmbientOcclusion, TexSpecular, TexLast
};

struct Texture {
	unsigned int id;
	std::set<TextureType> types;
	std::string path;
	int num_channels;
	Texture(const std::string& base_name) {
		this->base_name = base_name;
	}

	static std::string get_short_name_of_texture_type(TextureType texture_type) {
		if (texture_type == TexAlbedo) {
			return "alb";
		}
		else if (texture_type == TexNormal) {
			return "norm";
		}
		else if (texture_type == TexMetalness) {
			return "metal";
		}
		else if (texture_type == TexRoughness) {
			return "rough";
		}
		else if (texture_type == TexEmission) {
			return "emi";
		}
		else if (texture_type == TexAmbientOcclusion) {
			return "occ";
		}
		else {
			return "spec";
		}
	}

	static std::string get_long_name_of_texture_type(TextureType texture_type) {
		if (texture_type == TexAlbedo) {
			return "texture_albedo";
		}
		else if (texture_type == TexNormal) {
			return "texture_normal";
		}
		else if (texture_type == TexMetalness) {
			return "texture_metalness";
		}
		else if (texture_type == TexRoughness) {
			return "texture_roughness";
		}
		else if (texture_type == TexEmission) {
			return "texture_emission";
		}
		else if (texture_type == TexAmbientOcclusion) {
			return "texture_ambient_occlusion";
		}
		else {
			return "texture_specular";
		}
	}

	std::string get_name() {
		std::string output_name = base_name;
		for (auto it = types.begin(); it != types.end(); it++) {
			output_name += "_" + get_short_name_of_texture_type(*it);
		}
		return output_name;
	}
private:
	std::string base_name;
};

enum FileFormat {
	Default, glTF
};

struct Material {
	std::string name;
	std::map<TextureType, Texture*> textures;
	FileFormat format;

	Material(const std::string& name) {
		this->name = name;
	}
};

class Mesh {
public:
	//mesh date
	std::string name;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	Material* material;
	unsigned int VAO; 

	//constuctor
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material* material)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->material = material;
		setupMesh();  // set VAO,VBO,VEO
	}

	// render the mesh
	void Draw(Shader& shader, Material* draw_material, bool disable_depth_test)
	{
		if (disable_depth_test) {
			glDisable(GL_DEPTH_TEST);
		}

		const unsigned int OFFSET_TEXTURES = 3; // Accounting for PBR indirect light textures (irradiance, prefilter and BRDF maps)

		if (draw_material == nullptr) {
			draw_material = this->material;
		}

		shader.setInt("material_format", draw_material->format);

		int num_active_textures = 0;
		for (int type = TexAlbedo; type < TexLast; type++) {
			TextureType texture_type = (TextureType)type;
			std::string str_texture_type = Texture::get_long_name_of_texture_type(texture_type);
			if (draw_material->textures.find(texture_type) != draw_material->textures.end()) {
				Texture* texture = draw_material->textures[texture_type];
				glActiveTexture(GL_TEXTURE0 + OFFSET_TEXTURES + num_active_textures);
				shader.setInt(str_texture_type, OFFSET_TEXTURES + num_active_textures);
				shader.setInt("has_" + str_texture_type, true);
				glBindTexture(GL_TEXTURE_2D, texture->id);
				num_active_textures++;
			}
			else {
				shader.setInt("has_" + str_texture_type, false);
			}
		}


		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);

		if (disable_depth_test) {
			glEnable(GL_DEPTH_TEST);
		}
	}

private:
	unsigned int VBO, EBO;  //buffer 
	void setupMesh()
	{
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//vertex abbibute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		// vertex tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		// vertex bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

		glBindVertexArray(0);



	}

};


#endif // ! MESH_H
