///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// Initialize texture-related variables
	m_loadedTextures = 0;

	// Initialize texture array
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method loads textures from image files and configures
 *  them for use in the 3D scene
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// Load textures for the table surface
	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"table_surface");

	// Load textures for the vase (different parts)
	bReturn = CreateGLTexture(
		"../../Utilities/textures/gold-seamless-texture.jpg",
		"vase_bottom");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"vase_middle");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/circular-brushed-gold-texture.jpg",
		"vase_top");

	// Load texture for flower stems
	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"flower_stem");

	// Load texture for flower buds
	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainedglass.jpg",
		"flower_bud");

	// Load texture for pumpkin
	bReturn = CreateGLTexture(
		"../../Utilities/textures/abstract.jpg",
		"pumpkin");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/breadcrust.jpg",
		"pumpkin_stem");

	// Load second table texture for overlapping effect
	bReturn = CreateGLTexture(
		"../../Utilities/textures/cheddar.jpg",
		"table_overlay");

	// Load textures for the candles
	bReturn = CreateGLTexture(
		"../../Utilities/textures/amber_glass.jpg",
		"candle_holder");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/wax.jpg",
		"candle_wax");

	// Load texture for the book
	bReturn = CreateGLTexture(
		"../../Utilities/textures/book_cover.jpg",
		"book_cover");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/book_pages.jpg",
		"book_pages");

	// After the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Material for the table surface
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	tableMaterial.ambientStrength = 0.2f;
	tableMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f);
	tableMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tableMaterial.shininess = 32.0f;
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	// Material for the vase bottom part
	OBJECT_MATERIAL vaseBottomMaterial;
	vaseBottomMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.0f);
	vaseBottomMaterial.ambientStrength = 0.3f;
	vaseBottomMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.0f);
	vaseBottomMaterial.specularColor = glm::vec3(1.0f, 0.9f, 0.5f);
	vaseBottomMaterial.shininess = 64.0f;
	vaseBottomMaterial.tag = "vase_bottom";
	m_objectMaterials.push_back(vaseBottomMaterial);

	// Material for the vase middle part
	OBJECT_MATERIAL vaseMiddleMaterial;
	vaseMiddleMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	vaseMiddleMaterial.ambientStrength = 0.2f;
	vaseMiddleMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	vaseMiddleMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	vaseMiddleMaterial.shininess = 128.0f;
	vaseMiddleMaterial.tag = "vase_middle";
	m_objectMaterials.push_back(vaseMiddleMaterial);

	// Material for the vase top part
	OBJECT_MATERIAL vaseTopMaterial;
	vaseTopMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.0f);
	vaseTopMaterial.ambientStrength = 0.3f;
	vaseTopMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.0f);
	vaseTopMaterial.specularColor = glm::vec3(1.0f, 0.9f, 0.5f);
	vaseTopMaterial.shininess = 64.0f;
	vaseTopMaterial.tag = "vase_top";
	m_objectMaterials.push_back(vaseTopMaterial);

	// Material for the flower stems
	OBJECT_MATERIAL stemMaterial;
	stemMaterial.ambientColor = glm::vec3(0.1f, 0.3f, 0.1f);
	stemMaterial.ambientStrength = 0.2f;
	stemMaterial.diffuseColor = glm::vec3(0.2f, 0.6f, 0.2f);
	stemMaterial.specularColor = glm::vec3(0.1f, 0.3f, 0.1f);
	stemMaterial.shininess = 8.0f;
	stemMaterial.tag = "stem";
	m_objectMaterials.push_back(stemMaterial);

	// Material for the flower buds
	OBJECT_MATERIAL budMaterial;
	budMaterial.ambientColor = glm::vec3(0.3f, 0.1f, 0.3f);
	budMaterial.ambientStrength = 0.3f;
	budMaterial.diffuseColor = glm::vec3(0.7f, 0.2f, 0.7f);
	budMaterial.specularColor = glm::vec3(0.8f, 0.3f, 0.8f);
	budMaterial.shininess = 16.0f;
	budMaterial.tag = "bud";
	m_objectMaterials.push_back(budMaterial);

	// Material for the pumpkin
	OBJECT_MATERIAL pumpkinMaterial;
	pumpkinMaterial.ambientColor = glm::vec3(0.4f, 0.2f, 0.0f);
	pumpkinMaterial.ambientStrength = 0.3f;
	pumpkinMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.0f);
	pumpkinMaterial.specularColor = glm::vec3(0.5f, 0.4f, 0.1f);
	pumpkinMaterial.shininess = 16.0f;
	pumpkinMaterial.tag = "pumpkin";
	m_objectMaterials.push_back(pumpkinMaterial);

	// Material for the pumpkin stem
	OBJECT_MATERIAL pumpkinStemMaterial;
	pumpkinStemMaterial.ambientColor = glm::vec3(0.1f, 0.2f, 0.0f);
	pumpkinStemMaterial.ambientStrength = 0.2f;
	pumpkinStemMaterial.diffuseColor = glm::vec3(0.3f, 0.4f, 0.1f);
	pumpkinStemMaterial.specularColor = glm::vec3(0.2f, 0.3f, 0.1f);
	pumpkinStemMaterial.shininess = 4.0f;
	pumpkinStemMaterial.tag = "pumpkin_stem";
	m_objectMaterials.push_back(pumpkinStemMaterial);

	// Material for the candle holder
	OBJECT_MATERIAL candleHolderMaterial;
	candleHolderMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.0f);
	candleHolderMaterial.ambientStrength = 0.2f;
	candleHolderMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.1f);
	candleHolderMaterial.specularColor = glm::vec3(1.0f, 0.8f, 0.4f);
	candleHolderMaterial.shininess = 96.0f;
	candleHolderMaterial.tag = "candle_holder";
	m_objectMaterials.push_back(candleHolderMaterial);

	// Material for the candle wax
	OBJECT_MATERIAL candleWaxMaterial;
	candleWaxMaterial.ambientColor = glm::vec3(0.9f, 0.9f, 0.8f);
	candleWaxMaterial.ambientStrength = 0.2f;
	candleWaxMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 0.9f);
	candleWaxMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	candleWaxMaterial.shininess = 4.0f;
	candleWaxMaterial.tag = "candle_wax";
	m_objectMaterials.push_back(candleWaxMaterial);

	// Material for the book
	OBJECT_MATERIAL bookMaterial;
	bookMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	bookMaterial.ambientStrength = 0.1f;
	bookMaterial.diffuseColor = glm::vec3(0.5f, 0.3f, 0.1f);
	bookMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	bookMaterial.shininess = 8.0f;
	bookMaterial.tag = "book";
	m_objectMaterials.push_back(bookMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light source 1
	m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 10.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light source 2
	m_pShaderManager->setVec3Value("lightSources[1].position", -10.0f, 10.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light source 3
	m_pShaderManager->setVec3Value("lightSources[2].position", 1.0f, 10.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	//Light source 4
	m_pShaderManager->setVec3Value("lightSources[3].position", 10.0f, 0.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);

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
	// Load all textures
	LoadSceneTextures();

	//Define materials for objects
	DefineObjectMaterials();

	// Setup lighting for the scene
	SetupSceneLights();

	// Load the mesh shapes
	m_basicMeshes->LoadPlaneMesh(); // for table surface
	m_basicMeshes->LoadTaperedCylinderMesh(); // for the vase body
	m_basicMeshes->LoadCylinderMesh(); // for vase neck and flower stem
	m_basicMeshes->LoadSphereMesh(); // for flower buds
	m_basicMeshes->LoadBoxMesh(); // for pumpkin ridges
	m_basicMeshes->LoadConeMesh(); // for candle flame
	m_basicMeshes->LoadTorusMesh(); // for candle holder rim
	m_basicMeshes->LoadPrismMesh(); // for book binding
	m_basicMeshes->LoadPyramid4Mesh(); // for decorative element
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture tiling to the table surface (complex texturing technique)
	SetTextureUVScale(5.0f, 2.5f); // This tiles the texture 5x horizontally and 2.5x vertically
	SetShaderTexture("table_surface");

	// For the table surface
	SetShaderMaterial("table");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// Draw a second layer on top of the table with a different texture and transparency
	// This creates a complex overlapping texture effect
	scaleXYZ = glm::vec3(20.0f, 1.01f, 10.0f); // Slightly higher than table to avoid z-fighting
	positionXYZ = glm::vec3(0.0f, 0.01f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply a different texture with different tiling
	SetTextureUVScale(10.0f, 5.0f);
	SetShaderTexture("table_overlay");

	// For the table overlay (keeping transparency)
	SetShaderMaterial("table");

	// Make overlay partially transparent
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f);
	m_basicMeshes->DrawPlaneMesh();

	// Reset color and UV scale
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the vase body (bottom part - widest)
	scaleXYZ = glm::vec3(2.5f, 1.4f, 2.5f);  // Much wider and taller bottom
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.7f, 0.0f);  // Centered in the view

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture to vase bottom
	SetShaderTexture("vase_bottom");

	// For the vase bottom
	SetShaderMaterial("vase_bottom");

	m_basicMeshes->DrawTaperedCylinderMesh();

	// Draw the vase middle section (narrower)
	scaleXYZ = glm::vec3(1.2f, 1.0f, 1.2f);  // Narrower but still substantial
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.9f, 0.0f);  // Position above bottom part

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture to vase middle
	SetShaderTexture("vase_middle");

	// For the vase middle
	SetShaderMaterial("vase_middle");

	m_basicMeshes->DrawCylinderMesh();

	// Draw the vase top (wider opening)
	scaleXYZ = glm::vec3(1.8f, 0.6f, 1.8f);  // Dramatic flared opening
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.7f, 0.0f);  // Position above middle

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture to vase top
	SetShaderTexture("vase_top");

	// For the vase top
	SetShaderMaterial("vase_top");

	m_basicMeshes->DrawTaperedCylinderMesh();

	// Create many flower stems filling the vase in all directions
	// Generate 32 stems in a circular pattern
	for (int i = 0; i < 32; i++) {
		// Calculate varied parameters for natural look
		float angle = (i * 11.25f);  // Full 360° coverage (32 * 11.25 = 360)
		float stemLength = 1.0f + (i % 5) * 0.2f;  // Varying lengths
		float radialPosition = (i % 3) * 0.3f;  // Varied distances from center

		// Calculate stem position with random-like variation
		float xPos = radialPosition * sin(glm::radians(angle));
		float zPos = radialPosition * cos(glm::radians(angle));

		// Calculate tilt angle with variation
		float tiltAngle = 10.0f + (i % 4) * 5.0f;  // Varied tilt

		// Stem properties with variation
		scaleXYZ = glm::vec3(0.03f, stemLength, 0.03f);
		XrotationDegrees = tiltAngle;  // Store tilt for later calculations
		YrotationDegrees = angle;  // Direction matches position angle
		ZrotationDegrees = 0.0f;

		positionXYZ = glm::vec3(xPos, 3.0f, zPos);  // Start from top of vase

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Apply stem texture with UV scaling to make it look natural
		SetTextureUVScale(1.0f, 3.0f); // Stretch texture vertically along stem
		SetShaderTexture("flower_stem");

		// For the flower stems
		SetShaderMaterial("stem");


		m_basicMeshes->DrawCylinderMesh();

		// Reset UV scale
		SetTextureUVScale(1.0f, 1.0f);

		// Precisely calculate the end position of the stem
		// Convert angles to radians for calculation
		float tiltRad = glm::radians(tiltAngle);
		float angleRad = glm::radians(angle);

		// Calculate the vector representing the stem direction
		// This accounts for both X rotation (tilt) and Y rotation (angle)
		float dirX = sin(tiltRad) * sin(angleRad);
		float dirY = cos(tiltRad);
		float dirZ = sin(tiltRad) * cos(angleRad);

		// Multiply by stem length to get the endpoint offset
		float endX = stemLength * dirX;
		float endY = stemLength * dirY;
		float endZ = stemLength * dirZ;

		// Add to the stem base position to get final bud position
		float budX = xPos + endX;
		float budY = 3.3f + endY;
		float budZ = zPos + endZ;

		// Flower bud
		float budSize = 0.08f + (i % 4) * 0.03f;  // Slight size variation
		scaleXYZ = glm::vec3(budSize, budSize, budSize);

		positionXYZ = glm::vec3(budX, budY, budZ);

		SetTransformations(
			scaleXYZ,
			0.0f, 0.0f, 0.0f,
			positionXYZ);

		// Apply bud texture
		SetShaderTexture("flower_bud");

		// For the flower buds
		SetShaderMaterial("bud");

		m_basicMeshes->DrawSphereMesh();
	}

	// Draw the pumpkin body (spheroid with distinctive ridges)
	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);  // Wider than tall for squash shape
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;  // Rotate slightly for better view
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 0.5f, 2.0f);  // Position to the right front of vase

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply pumpkin texture
	SetShaderTexture("pumpkin");

	// For the pumpkin body
	SetShaderMaterial("pumpkin");

	m_basicMeshes->DrawSphereMesh();  // Base shape is a sphere

	// Create pumpkin ridges using thin, tall boxes arranged in a circle
	for (int i = 0; i < 8; i++) {
		float angle = i * 45.0f;  // 8 ridges evenly spaced

		// Calculate ridge position
		float ridgeX = 3.0f + 0.8f * sin(glm::radians(angle));
		float ridgeZ = 2.0f + 0.8f * cos(glm::radians(angle));

		// Ridge properties
		scaleXYZ = glm::vec3(0.1f, 0.9f, 0.1f);  // Thin, tall box
		XrotationDegrees = 0.0f;
		YrotationDegrees = angle;  // Orient toward center
		ZrotationDegrees = 15.0f;  // Tilt outward slightly

		positionXYZ = glm::vec3(ridgeX, 0.5f, ridgeZ);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Apply the same texture but darker to create subtle ridge effect
		SetShaderTexture("pumpkin");

		// For the pumpkin ridges
		SetShaderMaterial("pumpkin");

		m_basicMeshes->DrawBoxMesh();
	}

	// Add pumpkin stem
	scaleXYZ = glm::vec3(0.2f, 1.1f, 0.2f);
	XrotationDegrees = -60.0f;  // Stem tilts slightly
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 1.1f, 2.0f);  // Position on top of pumpkin

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply stem texture
	SetShaderTexture("pumpkin_stem");

	// For the pumpkin stem
	SetShaderMaterial("pumpkin_stem");

	m_basicMeshes->DrawCylinderMesh();

	// Drawing the first amber glass candle holder to the left of the vase
	// This is the main cylinder of the candle holder
	scaleXYZ = glm::vec3(0.8f, 0.6f, 0.8f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 0.3f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply candle holder texture
	SetShaderTexture("candle_holder");

	// For the candle holder material
	SetShaderMaterial("candle_holder");

	m_basicMeshes->DrawCylinderMesh();

	// Add a decorative torus rim to the top of the candle holder
	scaleXYZ = glm::vec3(0.85f, 0.85f, 0.2f);
	XrotationDegrees = 90.0f; // Orient torus horizontally
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 0.6f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply same texture as holder
	SetShaderTexture("candle_holder");

	// For the candle holder rim
	SetShaderMaterial("candle_holder");

	m_basicMeshes->DrawTorusMesh();

	// Add the candle wax inside the holder
	scaleXYZ = glm::vec3(0.5f, 0.3f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 0.7f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply candle wax texture
	SetShaderTexture("candle_wax");

	// For the candle wax material
	SetShaderMaterial("candle_wax");

	m_basicMeshes->DrawCylinderMesh();

	// Add a small flame using a cone
	scaleXYZ = glm::vec3(0.1f, 0.3f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 1.05f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set flame color (no texture)
	SetShaderColor(1.0f, 0.6f, 0.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();

	// Draw the second candle holder (similar but slightly different)
	// This is the main cylinder of the second candle holder
	scaleXYZ = glm::vec3(0.7f, 0.5f, 0.7f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f; // Rotate slightly for variation
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 0.25f, 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply candle holder texture
	SetShaderTexture("candle_holder");

	// For the candle holder material
	SetShaderMaterial("candle_holder");

	m_basicMeshes->DrawCylinderMesh();

	// Add a decorative torus rim to the top of the second candle holder
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.15f);
	XrotationDegrees = 90.0f; // Orient torus horizontally
	YrotationDegrees = 15.0f; // Match rotation with base
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 0.5f, 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply same texture as holder
	SetShaderTexture("candle_holder");

	// For the candle holder rim
	SetShaderMaterial("candle_holder");

	m_basicMeshes->DrawTorusMesh();

	// Add the candle wax inside the second holder
	scaleXYZ = glm::vec3(0.4f, 0.25f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f; // Match rotation with base
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 0.6f, 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply candle wax texture
	SetShaderTexture("candle_wax");

	// For the candle wax material
	SetShaderMaterial("candle_wax");

	m_basicMeshes->DrawCylinderMesh();

	// Add a small flame using a cone for the second candle
	scaleXYZ = glm::vec3(0.08f, 0.25f, 0.08f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f; // Match rotation with base
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 0.9f, 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set flame color (no texture)
	SetShaderColor(1.0f, 0.6f, 0.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();

	// Reset color after flame
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Draw a book on the table near the candles
	// Main book body
	scaleXYZ = glm::vec3(1.8f, 0.2f, 1.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f; // Angle the book slightly
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.5f, 0.1f, 0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply book cover texture
	SetShaderTexture("book_cover");

	// For the book material
	SetShaderMaterial("book");

	m_basicMeshes->DrawBoxMesh();

	// Book binding (uses prism shape)
	scaleXYZ = glm::vec3(0.2f, 0.2f, 1.2f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = -15.0f; // Match book angle
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.3f, 0.2f, 0.4f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply binding texture (same as cover)
	SetShaderTexture("book_cover");

	// For the book material
	SetShaderMaterial("book");

	m_basicMeshes->DrawPrismMesh();

	// Book pages (visible on the open side)
	scaleXYZ = glm::vec3(1.6f, 0.19f, 1.19f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f; // Match book angle
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.5f, 0.21f, 0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply pages texture
	SetShaderTexture("book_pages");

	// For the book material (same as cover)
	SetShaderMaterial("book");

	m_basicMeshes->DrawBoxMesh();

	// Add a decorative pyramid element to complete the scene
	scaleXYZ = glm::vec3(0.4f, 0.7f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.5f, 0.35f, -2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use a gold texture for the pyramid
	SetShaderTexture("vase_bottom");

	// Use gold material
	SetShaderMaterial("vase_bottom");

	m_basicMeshes->DrawPyramid4Mesh();
}