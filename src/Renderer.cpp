#include "Renderer.h"
#include "Renderer.h"
#include "Renderer.h"


Renderer::Renderer(int w, int h, std::string config_file)
{
	LoadConfigs(config_file);
	Initialize(w, h, config_file);
}

Renderer::Renderer(std::string config_file)
{
	LoadConfigs(config_file);
}

Renderer::Renderer()
{

}

void Renderer::LoadConfigs(std::string config_file)
{
	std::vector<fs::path> configs = GetFilesInFolder(fs::path(config_file).parent_path().string(), ".cfg");
	for (auto &file : configs)
	{
		rendering_configurations.push_back(file.filename().string());
	}
	config_folder = fs::path(config_file).parent_path().string();
}

void Renderer::Initialize(int w, int h, std::string config_f)
{
	main_textures.clear();
	shader_textures.clear();
	global_size.clear();
	shader_pipeline.clear();

	width = w;
	height = h;

	variables["width"] = width;
	variables["height"] = height;
	camera.SetResolution(vec2(w, h));
	camera.SetAspectRatio((float)w / (float)h);

	ExprParser parser;
	config_file = config_f;
	std::string compute_folder = fs::path(config_file).parent_path().generic_string();

	std::ifstream config(config_file);
	if (config.fail())
	{
		ERROR_MSG("Error opening pipeline configuration");
		return;
	}
	std::string line;

	int element = -1;
	int cur_shader = 0;

	

	std::vector<GLuint> stage_textures;
	std::string shader_file;
	vec2 global, tex_resolution;
	while (std::getline(config, line))
	{
		if (line.substr(0, 1) != "#")
		{
			parser.Parse(line);
			switch (element++)
			{
			case -1:
				for (int i = 0; i < parser.Evaluate(variables); i++)
				{
					main_textures.push_back(GenerateTexture(width, height));
				}
				break;
			case 0:
				shader_file = compute_folder + "/" + line;
				LoadShader(shader_file);
				break;
			case 1:
				global.x = ceil(parser.Evaluate(variables));
				break;
			case 2:
				global.y = ceil(parser.Evaluate(variables));
				break;
			case 3:
				tex_resolution.x = ceil(parser.Evaluate(variables));
				break;
			case 4:
				tex_resolution.y = ceil(parser.Evaluate(variables));
				break;
			case 5:
				for (int i = 0; i < parser.Evaluate(variables); i++)
				{
					stage_textures.push_back(GenerateTexture(tex_resolution.x, tex_resolution.y));
				}
				shader_textures.push_back(stage_textures);
				stage_textures.clear();
				global_size.push_back(global);
				element = 0;
				break;
			}
		}
	}

	config.close();
}

void Renderer::ReInitialize(int w, int h)
{
	main_textures.clear();
	shader_textures.clear();
	global_size.clear();

	width = w;
	height = h;
	variables["width"] = width;
	variables["height"] = height;
	camera.SetResolution(vec2(w, h));
	camera.SetAspectRatio((float)w / (float)h);

	ExprParser parser;

	std::string compute_folder = fs::path(config_file).parent_path().generic_string();

	std::ifstream config(config_file);
	if (config.fail())
	{
		ERROR_MSG("Error opening pipeline configuration");
		return;
	}
	std::string line;

	int element = -1;
	int cur_shader = 0;

	std::vector<GLuint> stage_textures;
	std::string shader_file;
	vec2 global, tex_resolution;
	while (std::getline(config, line))
	{
		if (line.substr(0, 1) != "#")
		{
			parser.Parse(line);
			switch (element++)
			{
			case -1:
				for (int i = 0; i < parser.Evaluate(variables); i++)
				{
					main_textures.push_back(GenerateTexture(width, height));
				}
				break;
			case 0:
				//shader_file = compute_folder + "/" + line;
			//	LoadShader(shader_file);
				break;
			case 1:
				global.x = ceil(parser.Evaluate(variables));
				break;
			case 2:
				global.y = ceil(parser.Evaluate(variables));
				break;
			case 3:
				tex_resolution.x = ceil(parser.Evaluate(variables));
				break;
			case 4:
				tex_resolution.y = ceil(parser.Evaluate(variables));
				break;
			case 5:
				for (int i = 0; i < parser.Evaluate(variables); i++)
				{
					stage_textures.push_back(GenerateTexture(tex_resolution.x, tex_resolution.y));
				}
				shader_textures.push_back(stage_textures);
				stage_textures.clear();
				global_size.push_back(global);
				element = 0;
				break;
			}
		}
	}

	config.close();
}


void Renderer::SetOutputTexture(sf::Texture & tex)
{
	shader_textures[shader_textures.size()-1][0] = tex.getNativeHandle();
}

void Renderer::LoadShader(std::string shader_file)
{
	shader_pipeline.push_back(ComputeShader(shader_file));
}

std::vector<std::string> Renderer::GetConfigurationsList()
{
	return rendering_configurations;
}

std::string Renderer::GetConfigFolder()
{
	return config_folder;
}

void Renderer::LoadExternalTextures(std::string texture_folder)
{
	std::vector<fs::path> images = GetFilesInFolder(texture_folder, ".png");
	for (auto &path : images)
	{
		sf::Texture textr; 
		textr.loadFromFile(path.string());
		input_textures.push_back(textr);
	}
}

void Renderer::Render()
{
	int stages = global_size.size();
	for (int i = 0; i < stages; i++)
	{
		int tex_id = 0;

		//bind textures from the previous step
		if (i != 0)
		{
			for (int j = 0; j < shader_textures[i - 1].size(); j++)
			{
				glBindImageTexture(tex_id++, shader_textures[i - 1][j], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
			}
		}

		//bind textures from the current step
		for (int j = 0; j < shader_textures[i].size(); j++)
		{
			glBindImageTexture(tex_id++, shader_textures[i][j], 0, GL_FALSE, 0, GL_READ_WRITE, (i == stages-1)?GL_RGBA8:GL_RGBA32F);
		}

		//global textures
		for (int j = 0; j < main_textures.size(); j++)
		{
			glBindImageTexture(tex_id++, main_textures[j], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		}

		for (auto &extr_text : input_textures)
		{
			glBindImageTexture(tex_id++, extr_text.getNativeHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		}
		
		shader_pipeline[i].setCamera(camera.GetGLdata());
		shader_pipeline[i].Run(global_size[i]);
	}
}

GLuint Renderer::GenerateTexture(float w, float h)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	//HDR texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return texture;
}
