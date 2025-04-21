///////////////////////////////////////////////////////////////////////////////
// object.cpp
// ============
// manage the objects in SceneManager
//
//  AUTHOR: Cade Bray - SNHU Student / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, April 7th, 2025
///////////////////////////////////////////////////////////////////////////////

#include "object.h"
#include "SceneManager.h"
using namespace glm;

/***********************************************************
 *  object()
 *
 *  The constructor for the class
 ***********************************************************/
object::object(SceneManager* sceneManager) : scenePtr(sceneManager)
{
	// Nothing to construct.
}

/***********************************************************
 *  ~object()
 *
 *  The deconstructor for the class
 ***********************************************************/
object::~object()
{
	// Object doesn't own sceneManager so we don't delete.
	
	// When deconstructor is called it will delete member functions.
}

/***********************************************************
 *  render()
 *
 *  Render the Object
 ***********************************************************/
void object::render()
{
	// Set the transformations
	scenePtr->SetTransformations(
		scale,
		rotations.x,
		rotations.y,
		rotations.z,
		position
	);

	// Set the Shaders using RGBA
	scenePtr->SetShaderColor(
		RGBA.x,
		RGBA.y,
		RGBA.z,
		RGBA.w
	);

	// Set the Shader Texture
	if (texture != "")
	{
		// Texture provided
		scenePtr->SetShaderTexture(texture);

		// Set the UV scale
		scenePtr->SetTextureUVScale(uvScale.x, uvScale.y);
	}

	// Set the Shader Material
	if (shaderMaterial != "")
	{
		// Set the Shader Material if it's not empty.
		scenePtr->SetShaderMaterial(shaderMaterial);
	}

	// Draw the shape with lambda function for draw mesh
	shape();
}

/***********************************************************
 *  resetAll()
 *
 *  Reset all vectors and texture. This will not reset lambda
 *  draw mesh function given.
 ***********************************************************/
void object::resetAll()
{
	uvScale = vec2(0.0f, 0.0f);
	rotations = vec3(0.0f, 0.0f, 0.0f);
	scale = vec3(1.0f, 1.0f, 1.0f);
	position = vec3(0.0f, 0.0f, 0.0f);
	RGBA = vec4(0.0f, 0.0f, 0.0f, 1);
	texture = "";
	shaderMaterial = "";
}

/***********************************************************
 *  set_uvScale()
 *
 *  Function for setting the uvScale of the object.
 ***********************************************************/
void object::set_uvScale(vec2 given_uvScale)
{
	uvScale = given_uvScale;
}

/***********************************************************
 *  setRotations()
 *
 *  Function for setting the rotations of the object.
 ***********************************************************/
void object::setRotations(vec3 givenRotations)
{
	rotations = givenRotations;
}

/***********************************************************
 *  setScale()
 *
 *  Function for setting the scale of the object.
 ***********************************************************/
void object::setScale(vec3 givenScale)
{
	scale = givenScale;
}

/***********************************************************
 *  setPosition()
 *
 *  Function for setting the position of the object.
 ***********************************************************/
void object::setPosition(vec3 givenPosition)
{
	position = givenPosition;
}

/***********************************************************
 *  setRGBA()
 *
 *  Function for setting the RGBA of the object.
 ***********************************************************/
void object::setRGBA(vec4 givenRGBA)
{
	RGBA = givenRGBA;
}

/***********************************************************
 *  setShape()
 *
 *  Function for setting the lamba shape of the object.
 ***********************************************************/
void object::setShape(std::function<void()> givenShape)
{
	shape = givenShape;
}

/***********************************************************
 *  setTexture()
 *
 *  Function for setting the texture of the object.
 ***********************************************************/
void object::setTexture(std::string givenTexture)
{
	texture = givenTexture;
}

/***********************************************************
 *  setTexture()
 *
 *  Function for setting the texture of the object.
 ***********************************************************/
void object::setObjectShaderMaterial(std::string givenMaterial)
{
	shaderMaterial = givenMaterial;
}