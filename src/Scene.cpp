#include "framework.h"
#include "Scene.h"
#include "Sphere.h"
#include "Camera.h"
#include <iostream>
#include "objModel.h"

/*
SceneUPtr Scene::Create()
{
	auto scene = SceneUPtr(new Scene());
	return std::move(scene);

}
*/
void Scene::build()
{
	vec3 eye = vec3(0, 0, 4), vup = vec3(0, 1, 0), lookat = vec3(0, 0, 0);
	float fov = 45 * 3.14159265 / 180.0f;
	camera.set(eye, lookat, vup, fov);

	La = vec3(0.4f, 0.4f, 0.4f);
	vec3 lightDirection(1, 1, 1), Le(2, 2, 2);
	lights.push_back(new Light(lightDirection, Le));

	vec3 ks(2, 2, 2);
	
	objects.push_back(new Sphere(vec3(-0.55, 0, 0), 0.5, 
						new RoughMaterial(vec3(0.3, 0.2, 0.1), ks, 50)));
	/*
	objects.push_back(new Sphere(vec3(0.55, 0,  0), 0.5, 
						new RoughMaterial(vec3(0.1, 0.2, 0.3), ks, 100)));
	objects.push_back(new Sphere(vec3(0, 0.5, -0.8), 0.5,
						new RoughMaterial(vec3(0.3, 0, 0.2), ks, 20)));
	
	objects.push_back(new Sphere(vec3(0, 0.3,  0.6), 0.5,
						new RefractiveMaterial(vec3(1.2, 1.2, 1.2))));
	*/
				
	objects.push_back(new Sphere(vec3(0.55, 0,  0), 0.5, 
						new ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));

	
	//ObjModel obj = new ObjModel{ std::string{ "cow" }, "D:/git/CG/opengl_example/model/cow.obj", new RoughMaterial(vec3(0.3, 0.2, 0.1), ks, 50) }; 
	//objects.push_back(obj);
}

void Scene::render(std::vector<vec4>& image) {
	for (int Y = 0; Y < WINDOW_HEIGHT; Y++) 
	{
		#pragma omp parallel for
		for (int X = 0; X < WINDOW_WIDTH; X++) 
		{
			vec3 color = trace(camera.getRay(X, Y), 0);
			//vec3 color = (0.2f, 0.2, 0.4f);
			image[Y * WINDOW_WIDTH + X] = vec4(color.x, color.y, color.z, 1);
			
		
		}
	}

}

Hit Scene::firstIntersect(Ray ray) 
{
	Hit bestHit;
	for (Intersectable * object : objects) {
		Hit hit = object->intersect(ray); //  hit.t < 0 if no intersection
		if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))  bestHit = hit;
	}
	if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
	return bestHit;
}

bool Scene::shadowIntersect(Ray ray) {	// for directional lights
	for (Intersectable * object : objects) if (object->intersect(ray).t > 0) return true;
	return false;
}

vec3 Scene::trace(Ray ray, int depth) {
	if (depth > 5) return La;
	Hit hit = firstIntersect(ray);
	if (hit.t < 0) return La;

	if (hit.material->type == ROUGH) {
		vec3 outRadiance = hit.material->ka * La;
		for (Light * light : lights) {
			Ray shadowRay(hit.position + hit.normal * epsilon, light->direction);
			float cosTheta = dot(hit.normal, light->direction);
			if (cosTheta > 0 && !shadowIntersect(shadowRay)) {	// shadow computation
				outRadiance = outRadiance + light->Le * hit.material->kd * cosTheta;
				vec3 halfway = normalize(-ray.dir + light->direction);
				float cosDelta = dot(hit.normal, halfway);
				if (cosDelta > 0) outRadiance = outRadiance + light->Le * hit.material->ks * powf(cosDelta, hit.material->shininess);
			}
		}
		return outRadiance;
	}

	float cosa = -dot(ray.dir, hit.normal);
	vec3 one(1, 1, 1);
	vec3 F = hit.material->F0 + (one - hit.material->F0) * pow(1 - cosa, 5);
	vec3 reflectedDir = ray.dir - hit.normal * dot(hit.normal, ray.dir) * 2.0f;
	vec3 outRadiance = trace(Ray(hit.position + hit.normal * epsilon, reflectedDir), depth + 1) * F;

	if (hit.material->type == REFRACTIVE) {
		float disc = 1 - (1 - cosa * cosa) / hit.material->ior / hit.material->ior; // scalar n
		if (disc >= 0) {
			vec3 refractedDir = ray.dir / hit.material->ior + hit.normal * (cosa / hit.material->ior - sqrt(disc));
			outRadiance = outRadiance +
				trace(Ray(hit.position - hit.normal * epsilon, refractedDir), depth + 1) * (one - F);
		}
	}
	return outRadiance;
}

