///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <functional>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include "object.h"

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.23f, 0.23f, 0.23f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL softMaterial;
	softMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	softMaterial.ambientStrength = 0.4f;
	softMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	softMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	softMaterial.shininess = 0.05;
	softMaterial.tag = "soft";

	m_objectMaterials.push_back(softMaterial);

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	wallMaterial.ambientStrength = 0.3f;
	wallMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	wallMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	wallMaterial.shininess = 0.5;
	wallMaterial.tag = "wall";

	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL matteMaterial;
	matteMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	matteMaterial.ambientStrength = 0.2f;
	matteMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	matteMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	matteMaterial.shininess = 0.0;
	matteMaterial.tag = "matte";

	m_objectMaterials.push_back(matteMaterial);

	OBJECT_MATERIAL screenMaterial;
	glassMaterial.ambientColor = glm::vec3(0.298f, 0.694f, 0.929f);
	glassMaterial.ambientStrength = 0.2f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 10.0;
	glassMaterial.tag = "screen";

	OBJECT_MATERIAL hedgeMaterial;
	hedgeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	hedgeMaterial.ambientStrength = 0.1f;
	hedgeMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.3f);
	hedgeMaterial.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	hedgeMaterial.shininess = 0.5;
	hedgeMaterial.tag = "hedge";

	m_objectMaterials.push_back(hedgeMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Room backlight
	m_pShaderManager->setVec3Value("lightSources[0].position", -3.0f, 10.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	// Room light
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 71.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// Outside light
	m_pShaderManager->setVec3Value("lightSources[2].position", 5.0f, 70.0f, -79.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);

	// Monitor light
	m_pShaderManager->setVec3Value("lightSources[3].position", -1.0f, 7.4f, -2.992f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.00f, 0.00f, 0.2f); // Blue light
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.0f, 0.0f, 0.8f); // Blue light
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.0f, 0.0f, 0.5f); // Blue light
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 50.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);

}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	bool bReturn = false;

	// CB: A lot of these textures aren't used but I loaded them so I can apply them to different
	// shapes to see how they look and stretch.

	// Load dark_ceramic
	bReturn = CreateGLTexture(
		"Textures/dark_ceramic.jpg",
		"dark_ceramic");

	// Load cement
	bReturn = CreateGLTexture(
		"Textures/cement.jpeg",
		"cement");

	// Load clouds
	bReturn = CreateGLTexture(
		"Textures/clouds.png",
		"clouds");

	// Load grass
	bReturn = CreateGLTexture(
		"Textures/grass.jpg",
		"grass");

	// Load drywall
	bReturn = CreateGLTexture(
		"Textures/drywall.jpg",
		"drywall");

	// Load dark_carpet
	bReturn = CreateGLTexture(
		"Textures/dark_carpet.jpg",
		"dark_carpet");

	// Load wood
	bReturn = CreateGLTexture(
		"Textures/wood.jpg",
		"wood");

	// Load green vegetation
	bReturn = CreateGLTexture(
		"Textures/green_vegetation.jpg",
		"green_vegetation");

	// Load marble brick
	bReturn = CreateGLTexture(
		"Textures/keys.jpg",
		"keys");

	// Load cobblestone
	bReturn = CreateGLTexture(
		"Textures/water.jpg",
		"water");

	// Load orange_brick
	bReturn = CreateGLTexture(
		"Textures/orange_brick.jpg",
		"orange_brick");

	// Load paper
	bReturn = CreateGLTexture(
		"Textures/paper.jpg",
		"paper");

	// Load pencil
	bReturn = CreateGLTexture(
		"Textures/pencil.jpg",
		"pencil");

	// Load homer
	bReturn = CreateGLTexture(
		"Textures/homer.gif",
		"homer");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();

	// add and define the light sources for the scene
	SetupSceneLights();

	// Load the textures for the 3D scenes
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Lambda calls to functions. I couldn't figure out how to pass just the
	// function or point to it so I used lambda instead.
	auto cone = [this]() {m_basicMeshes->DrawConeMesh();};
	auto cylinder = [this]() {m_basicMeshes->DrawCylinderMesh();};
	auto plane = [this]() {m_basicMeshes->DrawPlaneMesh();};
	auto box = [this]() {m_basicMeshes->DrawBoxMesh();};
	auto sphere = [this]() {m_basicMeshes->DrawSphereMesh();};
	auto half_sphere = [this]() {m_basicMeshes->DrawHalfSphereMesh();};
	auto torus = [this]() {m_basicMeshes->DrawTorusMesh();};
	auto half_torus = [this]() {m_basicMeshes->DrawHalfTorusMesh();};
	auto taper_cylinder = [this]() {m_basicMeshes->DrawTorusMesh();};

	// This is the general object that will be used. It will retain
	// values from previous assignments which makes working with repeat values
	// such as building multiple pencils easy.
	object* Object = new object(this);


	/******************************************************************/
	/*** Pencil cup                                                 ***/
	/******************************************************************/

	// Outer Cup White
	Object->setRotations(glm::vec3(3.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(2.0f, 4.0f, 2.0f)); // scale XYZ
	Object->setPosition(glm::vec3(13.0f, 1.0f, -3.0f)); // set position xyz
	Object->setShape(cylinder); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture("dark_ceramic"); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Inner Cup black
	Object->setScale(glm::vec3(1.7f, 4.01f, 1.7f)); // scale XYZ
	Object->setRGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture to nothing
	Object->render(); // Create the object.

	/******************************************************************/
	/*** Pencils                                                    ***/
	/******************************************************************/

	// Pencil Body 1
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.20f, 8.0f, 0.20f)); // scale XYZ
	Object->setPosition(glm::vec3(13.6f, 1.0f, -3.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.949f, 0.839f, 0.471f, 1)); // Shader RGBA
	Object->setObjectShaderMaterial("wood");
	Object->render(); // Create the object.

	// Pencil Body 2
	Object->setRotations(glm::vec3(0.0f, 0.0f, 15.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.20f, 7.0f, 0.20f)); // scale XYZ
	Object->setPosition(glm::vec3(12.7f, 1.0f, -2.80f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Body 3
	Object->setRotations(glm::vec3(0.0f, 0.0f, 10.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(12.9f, 1.0f, -3.40f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Body 4
	Object->setRotations(glm::vec3(10.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.20f, 6.4f, 0.20f)); // scale XYZ
	Object->setPosition(glm::vec3(13.3f, 1.0f, -2.6f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Body 5
	Object->setRotations(glm::vec3(10.0f, 0.0f, 10.0f)); // rotations as XYZ
	Object->render(); // Create the object.

	// Pencil Body 6
	Object->setRotations(glm::vec3(10.0f, 0.0f, 5.0f)); // rotations as XYZ
	Object->render(); // Create the object.

	// Pencil Cone 1
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.20f, 1.0f, 0.20f)); // scale XYZ
	Object->setPosition(glm::vec3(13.6f, 9.0f, -3.0f)); // set position xyz
	Object->setShape(cone); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.969f, 0.949f, 0.878f, 1)); // Shader RGBA
	Object->setTexture("wood"); // Setting texture
	Object->set_uvScale(glm::vec2(0.5f, 0.5f)); // uvscale of texture
	Object->render(); // Create the object.

	// Pencil Cone 2
	Object->setRotations(glm::vec3(0.0f, 0.0f, 10.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(11.68f, 7.9f, -3.4f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Cone 3
	Object->setRotations(glm::vec3(0.0f, 0.0f, 14.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(10.89f, 7.76f, -2.8f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Cone 4
	Object->setRotations(glm::vec3(10.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(13.3f, 7.32f, -1.49f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Cone 5
	Object->setRotations(glm::vec3(10.0f, 0.0f, 5.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(12.74f, 7.28f, -1.49f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil Cone 6
	Object->setRotations(glm::vec3(10.0f, 0.0f, 8.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(12.19f, 7.20f, -1.50f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil graphite 1
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.15f, 1.1f, 0.15f)); // scale XYZ
	Object->setPosition(glm::vec3(13.6f, 9.0f, -3.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0, 0, 0, 1)); // Shader RGBA to black
	Object->setTexture(""); // Setting texture to nothing
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Pencil graphite 2
	Object->setRotations(glm::vec3(0.0f, 0.0f, 10.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(11.68f, 7.9f, -3.40f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil graphite 3
	Object->setRotations(glm::vec3(0.0f, 0.0f, 14.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(10.89f, 7.76f, -2.8f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil graphite 4
	Object->setRotations(glm::vec3(10.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(13.3f, 7.32f, -1.49f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil graphite 5
	Object->setRotations(glm::vec3(10.0f, 0.0f, 5.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(12.74f, 7.28f, -1.49f)); // set position xyz
	Object->render(); // Create the object.

	// Pencil graphite 6
	Object->setRotations(glm::vec3(10.0f, 0.0f, 8.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(12.19f, 7.20f, -1.5f)); // set position xyz
	Object->render(); // Create the object.

	/******************************************************************/
	/*** Computer                                                   ***/
	/******************************************************************/

	//base
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(6.0f, 0.5f, 5.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-1.0f, 1.0f, -3.0f)); // set position xyz
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setShape(box);
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	//back base
	Object->setPosition(glm::vec3(-1.0f, 3.5f, -5.3f)); // set position xyz
	Object->setRGBA(glm::vec4(0.970f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setRotations(glm::vec3(100.0f, 0.0f, 0.0f)); // rotate to put up right
	Object->render();

	// attached back base
	Object->setScale(glm::vec3(6.0f, 0.5f, 2.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-1.0f, 5.7f, -3.8f)); // set position xyz
	Object->setRGBA(glm::vec4(0.970f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotate to put up right
	Object->render();

	//Monitor main
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(15.0f, 0.5f, 8.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-1.0f, 7.0f, -3.0f)); // set position xyz
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->render(); // Create the object.

	//Monitor black edges
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(14.9f, 0.49f, 7.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-1.0f, 7.4f, -2.992f)); // set position xyz
	Object->setRGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Shader RGBA
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	//Monitor viewing area
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(14.2f, 0.49f, 6.4f)); // scale XYZ
	Object->setPosition(glm::vec3(-1.0f, 7.4f, -2.991f)); // set position xyz
	Object->setRGBA(glm::vec4(0.3f, 0.5f, 0.2f, 1.0f)); // Shader RGBA
	Object->setTexture("homer");
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Keyboard
	Object->setRotations(glm::vec3(7.0f, 5.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(10.0f, 1.0f, 3.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-2.0f, 1.0f, 3.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// Keyboard keys
	Object->setRotations(glm::vec3(7.0f, 5.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(9.9f, 1.01f, 2.9f)); // scale XYZ
	Object->setPosition(glm::vec3(-2.0f, 1.0f, 3.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture("keys"); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// mouse
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(1.5f, 1.0f, 2.0f)); // scale XYZ
	Object->setPosition(glm::vec3(6.0f, 1.0f, 3.0f)); // set position xyz
	Object->setShape(half_sphere); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// mouse button
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(1.15f, 0.805f, 0.4f)); // scale XYZ
	Object->setPosition(glm::vec3(6.0f, 1.0f, 2.25f)); // set position xyz
	Object->setShape(half_torus); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("matte");
	Object->render(); // Create the object.

	/******************************************************************/
	/*** Draw box for the surface our objects will sit on.          ***/
	/******************************************************************/

	// Cup on Desk
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(1.2f, 2.5f, 1.2f)); // scale XYZ
	Object->setPosition(glm::vec3(-10.0f, 1.0f, 0.0f)); // set position xyz
	Object->setShape(cylinder); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// Cup handle
	Object->setRotations(glm::vec3(90.0f, 90.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(0.7f, 0.9f, 0.7f)); // scale XYZ
	Object->setPosition(glm::vec3(-10.0f, 2.3f, 1.0f)); // set position xyz
	Object->setShape(half_torus); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// water in cup
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(1.0f, 2.51f, 1.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-10.0f, 1.0f, 0.0f)); // set position xyz
	Object->setShape(cylinder); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Shader RGBA
	Object->setTexture("water"); // Setting texture
	Object->set_uvScale(glm::vec2(1.0f, 1.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Book 1 on Desk
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(5.0f, 1.0f, 6.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-16.0f, 1.0f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.44f, 0.23f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// Book 1 paper
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(4.81f, 0.4f, 6.1f)); // scale XYZ
	Object->setPosition(glm::vec3(-15.9f, 1.27f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Book 2 on Desk
	Object->setRotations(glm::vec3(0.0f, 15.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(5.0f, 0.45f, 6.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-16.0f, 1.75f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 0.7f, 0.22f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("wood");
	Object->render(); // Create the object.

	// Book 2 paper
	Object->setRotations(glm::vec3(0.0f, 15.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(4.81f, 0.4f, 6.1f)); // scale XYZ
	Object->setPosition(glm::vec3(-15.9f, 1.75f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Book 1 on Desk
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(5.0f, 0.45f, 6.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-16.0f, 2.20f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.44f, 0.23f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture("drywall"); // Setting texture
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// Book 1 paper
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(4.81f, 0.4f, 6.1f)); // scale XYZ
	Object->setPosition(glm::vec3(-15.9f, 2.20f, -4.0f)); // set position xyz
	Object->setShape(box); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Desktop
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(40.0f, 2.0f, 20.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 0.0f, 0.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.773f, 0.78f, 0.702f, 1)); // Shader RGBA to black
	Object->setShape(box);
	Object->setTexture("wood"); // Setting texture to wood
	Object->set_uvScale(glm::vec2(3.0f, 3.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("wood");
	Object->render(); // Create the object.

	// FR Desk Leg
	Object->setRotations(glm::vec3(180.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(1.0f, 18.0f, 1.0f)); // scale XYZ
	Object->setPosition(glm::vec3(18.0f, 0.0f, 8.0f)); // set position xyz
	Object->setShape(cylinder); // Set Lambda draw mesh function
	Object->setRGBA(glm::vec4(0.369f, 0.369f, 0.369f, 1)); // Shader RGBA
	Object->setTexture(""); // Setting texture
	Object->setObjectShaderMaterial("metal");
	Object->render(); // Create the object.

	// FL Desk Leg
	Object->setPosition(glm::vec3(-18.0f, 0.0f, 8.0f)); // set position xyz
	Object->render(); // Create the object.

	// RL Desk Leg
	Object->setPosition(glm::vec3(-18.0f, 0.0f, -8.0f)); // set position xyz
	Object->render(); // Create the object.

	// RR Desk Leg
	Object->setPosition(glm::vec3(18.0f, 0.0f, -8.0f)); // set position xyz
	Object->render(); // Create the object.

	/******************************************************************/
	/*** Room                                                       ***/
	/******************************************************************/

	// North Wall 1
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(20.0f, 1.0f, 25.0f)); // scale XYZ
	Object->setPosition(glm::vec3(60.0f, 26.0f, -50.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.612f, 0.612f, 0.612f, 1)); // Shader RGBA
	Object->setShape(plane);
	Object->setTexture("drywall"); // Setting texture to drywall
	Object->set_uvScale(glm::vec2(3.0f, 3.0f)); // uvscale of texture
	Object->setObjectShaderMaterial("wall");
	Object->render(); // Create the object.

	// South Wall
	Object->setScale(glm::vec3(80.0f, 1.0f, 44.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 26.0f, 80.0f)); // set position xyz
	Object->render(); // Create the object.

	// North Wall 2
	Object->setScale(glm::vec3(20.0f, 1.0f, 25.0f)); // scale XYZ
	Object->setPosition(glm::vec3(-60.0f, 26.0f, -50.0f)); // set position xyz
	Object->render(); // Create the object.

	// North Wall 3
	Object->setScale(glm::vec3(80.0f, 1.0f, 10.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, -8.0f, -50.0f)); // set position xyz
	Object->render(); // Create the object.

	// North Wall 4
	Object->setPosition(glm::vec3(0.0f, 60.0f, -50.0f)); // set position xyz
	Object->set_uvScale(glm::vec2(5.0f, 1.3f)); // uvscale of texture
	Object->render(); // Create the object.

	// East Wall
	Object->setRotations(glm::vec3(0.0f, 0.0f, 90.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(44.0f, 1.0f, 65.0f)); // scale XYZ
	Object->set_uvScale(glm::vec2(3.0f, 3.0f)); // uvscale of texture
	Object->setPosition(glm::vec3(80.0f, 26.0f, 15.0f)); // set position xyz
	Object->render(); // Create the object.

	// West Wall
	Object->setPosition(glm::vec3(-80.0f, 26.0f, 15.0f)); // set position xyz
	Object->render(); // Create the object.

	// Ceiling
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(80.0f, 1.0f, 65.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 70.0f, 15.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.467f, 0.467f, 0.58f, 1)); // Shader RGBA
	Object->setTexture(""); // Setting texture to nothing
	Object->render(); // Create the object.

	// Ceiling Light
	Object->setShape(sphere);
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(5.0f, 5.0f, 5.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 71.0f, 0.0f)); // set position xyz
	Object->setRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1)); // Shader RGBA
	Object->setTexture(""); // Setting texture to nothing
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Ceiling Light torus
	Object->setShape(torus);
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(0.0f, 70.0f, 0.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1)); // Shader RGBA
	Object->setObjectShaderMaterial("matte");
	Object->render(); // Create the object.

	// Floor
	Object->setShape(plane);
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(80.0f, 1.0f, 65.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, -18.0f, 15.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.467f, 0.467f, 0.58f, 1)); // Shader RGBA
	Object->setTexture("dark_carpet"); // Setting texture to nothing
	Object->set_uvScale(glm::vec2(7.0f, 7.0f));
	Object->setObjectShaderMaterial("matte");
	Object->render(); // Create the object.

	/******************************************************************/
	/*** Outside                                                    ***/
	/******************************************************************/

	// Sky 1
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(100.0f, 1.0f, 100.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 0.0f, -80.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.725f, 0.859f, 0.988f, 1)); // Shader RGBA
	Object->setTexture("clouds"); // Setting texture to nothing
	Object->set_uvScale(glm::vec2(3.0f, 3.0f));
	Object->setObjectShaderMaterial("glass");
	Object->render(); // Create the object.

	// Sky 2
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(0.0f, 71.0f, -80.0f)); // set position xyz
	Object->render(); // Create the object.

	// Sky 3
	Object->setRotations(glm::vec3(0.0f, 0.0f, 90.0f)); // rotations as XYZ
	Object->setPosition(glm::vec3(-81.0f, 0.0f, -80.0f)); // set position xyz
	Object->render(); // Create the object.

	// Sky 4
	Object->setPosition(glm::vec3(81.0f, 0.0f, -80.0f)); // set position xyz
	Object->render(); // Create the object.

	// Brick wall
	Object->setRotations(glm::vec3(90.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(100.0f, 1.0f, 18.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 0.0f, -79.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.961f, 0.329f, 0.329f, 1)); // Shader RGBA
	Object->setTexture("orange_brick"); // Setting texture to nothing
	Object->setObjectShaderMaterial("wall");
	Object->render(); // Create the object.

	// Hedge
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(200.0f, 15.0f, 5.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, -10.0f, -79.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.318f, 0.961f, 0.094f, 0.5)); // Shader RGBA
	Object->setTexture("green_vegetation"); // Setting texture to nothing
	Object->set_uvScale(glm::vec2(5.0f, 1.0f));
	Object->setShape(box);
	Object->setObjectShaderMaterial("hedge");
	Object->render(); // Create the object.

	// Brick wall topper
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(200.0f, 5.0f, 5.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, 19.0f, -79.0f)); // set position xyz
	Object->setRGBA(glm::vec4(1, 1, 1, 1)); // Shader RGBA
	Object->setTexture("cement"); // Setting texture to nothing
	Object->set_uvScale(glm::vec2(3.0f, 3.0f));
	Object->setObjectShaderMaterial("soft");
	Object->render(); // Create the object.

	// Outside Ground
	Object->setRotations(glm::vec3(0.0f, 0.0f, 0.0f)); // rotations as XYZ
	Object->setScale(glm::vec3(100.0f, 1.0f, 100.0f)); // scale XYZ
	Object->setPosition(glm::vec3(0.0f, -19.0f, -80.0f)); // set position xyz
	Object->setRGBA(glm::vec4(0.318f, 0.961f, 0.094f, 1)); // Shader RGBA
	Object->setTexture("green_vegetation"); // Setting texture to nothing
	Object->setShape(plane);
	Object->set_uvScale(glm::vec2(30.0f, 30.0f));
	Object->setObjectShaderMaterial("matte");
	Object->render(); // Create the object.

	// Free up memory.
	delete Object;
	/****************************************************************/
}
