///////////////////////////////////////////////////////////////////////////////
// object.h
// ============
// manage the objects in SceneManager
//
//  AUTHOR: Cade Bray - SNHU Student / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, April 7th, 2025
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SceneManager.h"
#include <functional>
#include <string>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class object
{
public:
	object(SceneManager* sceneManager);
	~object();
	void render();
	void resetAll();
	void set_uvScale(glm::vec2 given_uvScale);
	void setRotations(glm::vec3 givenRotations);
	void setScale(glm::vec3 givenScale);
	void setPosition(glm::vec3 givenPosition);
	void setRGBA(glm::vec4 givenRGBA);
	void setShape(std::function<void()> givenShape);
	void setTexture(std::string givenTexture);

	void setObjectShaderMaterial(std::string givenMaterial);

private:
	// Pointer back to the SceneManager instance so we can call
	// the methods in that class.
	SceneManager* scenePtr;

	// Member variables used for setting transformations and more
	glm::vec2 uvScale = glm::vec2(0.0f, 0.0f);
	glm::vec3 rotations = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec4 RGBA = glm::vec4(0.0f, 0.0f, 0.0f, 1);
	std::function<void()> shape = [this]() {scenePtr->m_basicMeshes->DrawBoxMesh();};
	std::string texture = "";
	std::string shaderMaterial = "";
};