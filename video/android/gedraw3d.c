/*
	The Gamma Engine Library is a multiplatform library made to make games
	Copyright (C) 2012  Aubry Adrien (dridri85)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../ge_internal.h"

static ge_Scene* current_scene;
extern ge_Camera* ge_current_camera;
extern u32 shadow_map;
extern ge_Shader* ge_line_shader;
extern ge_Shader* ge_current_shader;
void DrawClouds(ge_Scene* scene);

void geLoadMatrix(float* m);
void ge_LoadIdentity(float* m);
void ge_Translate(float* m, float x, float y, float z);
void ge_Rotate(float* m, float x, float y, float z);
void ge_Scale(float* m, float x, float y, float z);

void geObjectMatrixIdentity(ge_Object* obj){
	ge_LoadIdentity(obj->matrix);
	obj->matrix_used = false;
}

void geObjectMatrixLoad(ge_Object* obj, float* m){
	memcpy(obj->matrix, m, sizeof(float)*16);
	obj->matrix_used = true;
}

void geObjectMatrixRotate(ge_Object* obj, float x, float y, float z){
	ge_Rotate(obj->matrix, x, y, z);
	obj->matrix_used = true;
}

void geObjectMatrixTranslate(ge_Object* obj, float x, float y, float z){
	ge_Translate(obj->matrix, x, y, z);
	obj->matrix_used = true;
}

void geObjectMatrixScale(ge_Object* obj, float x, float y, float z){
	ge_Scale(obj->matrix, x, y, z);
	obj->matrix_used = true;
}

static void multMat4Vec4(ge_Vector3f* ret, float* mat, ge_Vector4f* vec){
	ret->x  = vec->x*mat[0] + vec->y*mat[4] + vec->z*mat[8]  + vec->w*mat[12];
	ret->y  = vec->x*mat[1] + vec->y*mat[5] + vec->z*mat[9]  + vec->w*mat[13];
	ret->z  = vec->x*mat[2] + vec->y*mat[6] + vec->z*mat[10] + vec->w*mat[14];
	float w = vec->x*mat[3] + vec->y*mat[7] + vec->z*mat[11] + vec->w*mat[15];
	ret->x /= w;
	ret->y /= w;
	ret->z /= w;
}
static void multMat3Vec3(ge_Vector3f* ret, float* mat, ge_Vector4f* vec){
	ret->x = mat[0]*vec->x + mat[4]*vec->y + mat[8]*vec->z;
	ret->y = mat[1]*vec->x + mat[5]*vec->y + mat[9]*vec->z;
	ret->z = mat[2]*vec->x + mat[6]*vec->y + mat[10]*vec->z;
}

static void StaticLightingFunc2(ge_Scene* scene, ge_Renderer* render, int current_object){
	int i = 0;
	float* mview = geGetMatrix(GE_MATRIX_VIEW);
	for(i=0; i<scene->nLights; i++){
		multMat4Vec4(&scene->lights[i].vector, mview, &scene->lights[i].position);
		multMat3Vec3(&scene->lights[i].target_vector, mview, &scene->lights[i].target);
	}
}

void geRendererUse(ge_Renderer* render){
	geShaderUse(render->shader);
	if(ge_current_camera){
		glUniform3f(render->shader->loc_camera, ge_current_camera->x, ge_current_camera->y, ge_current_camera->z);
	}
	geShaderUniform1f(render->shader->loc_time, ((float)geGetTick()) / 1000.0);

	glBindBuffer(GL_ARRAY_BUFFER, render->vbo);




	if(render->customVert){
		if(render->customVert->vertex_offset >= 0){
			glEnableVertexAttribArray(3); //pos
			glVertexAttribPointer(3, render->customVert->vertex_count, render->customVert->vertex_type, false, render->customVert->size, BUFFER_OFFSET(render->customVert->vertex_offset));
		}else{
			glDisableVertexAttribArray(3); //pos
		}
		if(render->customVert->color_offset >= 0){
			glEnableVertexAttribArray(1); //color
			glVertexAttribPointer(1, render->customVert->color_count, render->customVert->color_type, true, render->customVert->size, BUFFER_OFFSET(render->customVert->color_offset));
		}else{
			glDisableVertexAttribArray(1); //color
		}
		if(render->customVert->texture_offset >= 0){
			glEnableVertexAttribArray(0); //tex
			glVertexAttribPointer(0, render->customVert->texture_count, render->customVert->texture_type, false, render->customVert->size, BUFFER_OFFSET(render->customVert->texture_offset));
		}else{
			glDisableVertexAttribArray(0); //tex
		}
		if(render->customVert->normal_offset >= 0){
			glEnableVertexAttribArray(2); //normal
			glVertexAttribPointer(2, 3, render->customVert->normal_type, true, render->customVert->size, BUFFER_OFFSET(render->customVert->normal_offset));
		}else{
			glDisableVertexAttribArray(2); //normal
		}
	}else{
		glEnableVertexAttribArray(0); //tex
		glEnableVertexAttribArray(1); //color
		glEnableVertexAttribArray(2); //normal
		glEnableVertexAttribArray(3); //pos
		
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(ge_Vertex), BUFFER_OFFSET(0));
		glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(ge_Vertex), BUFFER_OFFSET(12));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(ge_Vertex), BUFFER_OFFSET(28));
		glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(ge_Vertex), BUFFER_OFFSET(40));

	}

	if(render->depth_enabled){
		glEnable(GL_DEPTH_TEST);
	}else{
		glDisable(GL_DEPTH_TEST);
	}
	glDepthMask(render->depth_mask);
	if(render->blend_enabled){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}else{
		glDisable(GL_BLEND);
	}
	if(render->ext_func){
		render->ext_func(render, -1);
	}
}

void geRenderObjects(ge_Renderer* render){
	geRendererUse(render);

	geMatrixMode(GE_MATRIX_MODEL);
	geLoadIdentity();

	int i=0;
	int j=0;
//	int current_tex_id[8] = { 0 };
	int default_matrix_ok = false;
	int update_matrices = true;

	for(i=0; i<render->nObjs; i++){
		ge_Object* obj = render->objs[i];
	//	gePrintDebug(0x100, "Object %d/%d \"%s\", vert_start: %d, nVerts: %d\n", i+1, render->nObjs, obj->name, obj->vert_start, obj->nVerts);
		if(obj->vert_start<0)continue;

		if(obj->matrix_used){
			geLoadMatrix(obj->matrix);
			default_matrix_ok = false;
			update_matrices = true;
		}else if(!default_matrix_ok){
			geLoadIdentity();
			default_matrix_ok = true;
			update_matrices = true;
		}

		if(render->ext_func){
			render->ext_func(render, i);
		}

		if(update_matrices){
		//	CalculateModelMatrices();
			geUpdateMatrix();
		//	StaticLightingFunc2(current_scene, render, -1);
			update_matrices = false;
		}
		
		for(j=7; j>=0; j--){
			if(obj->material.textures[j]){
				if(obj->material.textures[j]->flags & GE_IMAGE_3D){
					/*
					glActiveTexture(GL_TEXTURE0+j);
					glEnable(GL_TEXTURE_3D);
					glBindTexture(GL_TEXTURE_3D, obj->material.textures[j]->id);
					*/
				}else{
					glActiveTexture(GL_TEXTURE0+j);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, obj->material.textures[j]->id);
				}
			}else{
				glActiveTexture(GL_TEXTURE0+j);
				glDisable(GL_TEXTURE_2D);
			//	break;
			}
		}

	
		glDrawArrays(GL_TRIANGLES, render->objs[i]->vert_start, render->objs[i]->nVerts);

		if(ge_line_shader){
			//todo
		}
	}
}

void geObjectDraw(ge_Object* obj){
/*
	geMatrixMode(GE_MATRIX_MODEL);
	geLoadMatrix(obj->matrix);
	CalculateModelMatrices();
	geUpdateMatrix();

	if(obj->material.textures[0]){
		glBindTexture(GL_TEXTURE_2D, obj->material.textures[0]->id);
	}else{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDrawArrays(GL_TRIANGLES, obj->vert_start, obj->nVerts);
*/

	if(obj->material.textures[0]){
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, obj->material.textures[0]->id);
	}else{
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDrawArrays(GL_TRIANGLES, obj->vert_start, obj->nVerts);
}

void geDrawArray(int mode, int start, int num){
	glDrawArrays(mode, start, num);
}

void geRendererUpdate(ge_Renderer* render){
	if(!render)return;
	int i = 0;
	int j = 0;
	float* mview = geGetMatrix(GE_MATRIX_VIEW);
	
	geShaderUse(render->shader);
	for(i=0; i<render->nDynamicLights; i++){
		ge_Light* light = render->lights[i];
		j = light->i_loc;
		glUniform1i(render->shader->loc_lights[j].loc_flags, light->flags);
		glUniform4f(render->shader->loc_lights[j].loc_position, light->position.x, light->position.y, light->position.z, light->position.w);
		glUniform4f(render->shader->loc_lights[j].loc_target, light->target.x, light->target.y, light->target.z, light->target.w);
		glUniform4f(render->shader->loc_lights[j].loc_ambient, light->ambient[0], light->ambient[1], light->ambient[2], light->ambient[3]);
		glUniform4f(render->shader->loc_lights[j].loc_diffuse, light->diffuse[0], light->diffuse[1], light->diffuse[2], light->diffuse[3]);
		glUniform4f(render->shader->loc_lights[j].loc_specular, light->specular[0], light->specular[1], light->specular[2], light->specular[3]);
		glUniform1f(render->shader->loc_lights[j].loc_attenuation, light->attenuation);
		glUniform1f(render->shader->loc_lights[j].loc_spotCutoff, light->spot_cutoff);
		glUniform1f(render->shader->loc_lights[j].loc_spotCosCutoff, light->spot_coscutoff);
		glUniform1f(render->shader->loc_lights[j].loc_spotExponent, light->spot_exponent);
		glActiveTexture(GL_TEXTURE7);
		glEnable(GL_TEXTURE_2D);
		glUniform1i(render->shader->loc_lights[j].loc_shadow, 7);
	}
	
	for(i=0; i<render->nLights; i++){
		ge_Light* light = render->lights[i+8/*j+scene->nDynamicLights*/];
		multMat4Vec4(&light->vector, mview, &light->position);
		multMat3Vec3(&light->target_vector, mview, &light->target);
		j = light->i_loc;
		glUniform1i(render->shader->loc_lights[j].loc_flags, light->flags);
		glUniform4f(render->shader->loc_lights[j].loc_position, light->position.x, light->position.y, light->position.z, light->position.w);
		glUniform4f(render->shader->loc_lights[j].loc_target, light->target.x, light->target.y, light->target.z, light->target.w);
		glUniform4f(render->shader->loc_lights[j].loc_ambient, light->ambient[0], light->ambient[1], light->ambient[2], light->ambient[3]);
		glUniform4f(render->shader->loc_lights[j].loc_diffuse, light->diffuse[0], light->diffuse[1], light->diffuse[2], light->diffuse[3]);
		glUniform4f(render->shader->loc_lights[j].loc_specular, light->specular[0], light->specular[1], light->specular[2], light->specular[3]);
		if(!light->used){
			glUniform1f(render->shader->loc_lights[j].loc_attenuation, -1.0);
		}else if(light->type != GE_LIGHT_TYPE_SPOT){
			glUniform1f(render->shader->loc_lights[j].loc_attenuation, -2.0);
		}else{
			glUniform1f(render->shader->loc_lights[j].loc_attenuation, light->attenuation);
		}
		glUniform1f(render->shader->loc_lights[j].loc_spotCutoff, light->spot_cutoff);
		glUniform1f(render->shader->loc_lights[j].loc_spotCosCutoff, light->spot_coscutoff);
		glUniform1f(render->shader->loc_lights[j].loc_spotExponent, light->spot_exponent);
		glUniform3f(render->shader->loc_lights[j].loc_vector, light->vector.x, light->vector.y, light->vector.z);
		glUniform3f(render->shader->loc_lights[j].loc_targetVector, light->target_vector.x, light->target_vector.y, light->target_vector.z);
		glUniform1f(render->shader->loc_lights[j].loc_CosInnerMinusOuterAngle, light->CosInnerMinusOuterAngle);
		glUniform1f(render->shader->loc_lights[j].loc_CosOuterConeAngle, light->CosOuterConeAngle);
		glActiveTexture(GL_TEXTURE7);
		glEnable(GL_TEXTURE_2D);
		if(light->shadow)glBindTexture(GL_TEXTURE_2D, light->shadow->id);
		glUniform1i(render->shader->loc_lights[j].loc_shadow, 7);
	}
}

void geSceneDraw(ge_Scene* scene){
	current_scene = scene;
	int i = 0;
	int j = 0;
	CalculateModelMatrices();
	/*
	if(scene->sky.animator){
		geRendererUse(&scene->sky);
		geShaderUniform1f(scene->sky.depth_enabled, ((float)geGetTick())/1000.0);
		geMatrixMode(GE_MATRIX_MODEL);
		geLoadIdentity();
		geUpdateMatrix();
		for(i=0; i< scene->sky.nObjs; i++){
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scene->sky.objs[i]->material.textures[0]->id);
			if(scene->sky.objs[i]->material.textures[1]){
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, scene->sky.objs[i]->material.textures[1]->id);
			//	geShaderUniform1i(geShaderUniformID(scene->sky.shader, "ge_Texture1"), 1);
			}
			glDrawArrays(GL_TRIANGLES, scene->sky.objs[i]->vert_start, scene->sky.objs[i]->nVerts);
		}
	}
	*/
	if(scene->cloudsGenerator){
	}

	float time = ((float)geGetTick()) / 1000.0;

	for(i=0; i<scene->nRenderers; i++){
		if(scene->renderers[i].enabled == false)continue;
		if(scene->renderers[i].matrix_used){
			geMatrixMode(GE_MATRIX_PROJECTION);
			geLoadMatrix(scene->renderers[i].projection_matrix);
		}else{
			geMatrixMode(GE_MATRIX_PROJECTION);
			geLoadMatrix(libge_context->projection_matrix);
		}
		geShaderUse(scene->renderers[i].shader);
		glUniform1f(scene->renderers[i].shader->loc_time, time);
		glUniform1f(scene->renderers[i].shader->loc_ratio, ((float)libge_context->width) / ((float)libge_context->height));
		if(scene->fogEnabled){
			glUniform1f(scene->renderers[i].shader->loc_fog_density, scene->fog->density);
			glUniform4f(scene->renderers[i].shader->loc_fog_color, scene->fog->color[0], scene->fog->color[1], scene->fog->color[2], scene->fog->color[3]);
			glUniform1f(scene->renderers[i].shader->loc_fog_start, scene->fog->start);
			glUniform1f(scene->renderers[i].shader->loc_fog_end, scene->fog->end);
		}
		/*
		if(scene->renderers[i].depth_enabled){
			glEnable(GL_DEPTH_TEST);
		}else{
			glDisable(GL_DEPTH_TEST);
		}
		if(scene->renderers[i].blend_enabled){
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}else{
			glDisable(GL_BLEND);
		}
		*/
		if(scene->renderers[i].ext_func){
			scene->renderers[i].ext_func(&scene->renderers[i], -1);
		}
		StaticLightingFunc2(scene, &scene->renderers[i], -1);
		for(j=0; j<scene->nDynamicLights; j++){
			glUniform4f(scene->renderers[i].shader->loc_lights[j].loc_position, scene->dynamicLights[j].position.x, scene->dynamicLights[j].position.y, scene->dynamicLights[j].position.z, scene->dynamicLights[j].position.w);
			glUniform4f(scene->renderers[i].shader->loc_lights[j].loc_target, scene->dynamicLights[j].target.x, scene->dynamicLights[j].target.y, scene->dynamicLights[j].target.z, scene->dynamicLights[j].target.w);
			glUniform4f(scene->renderers[i].shader->loc_lights[j].loc_ambient, scene->dynamicLights[j].ambient[0], scene->dynamicLights[j].ambient[1], scene->dynamicLights[j].ambient[2], scene->dynamicLights[j].ambient[3]);
			glUniform4f(scene->renderers[i].shader->loc_lights[j].loc_diffuse, scene->dynamicLights[j].diffuse[0], scene->dynamicLights[j].diffuse[1], scene->dynamicLights[j].diffuse[2], scene->dynamicLights[j].diffuse[3]);
			glUniform4f(scene->renderers[i].shader->loc_lights[j].loc_specular, scene->dynamicLights[j].specular[0], scene->dynamicLights[j].specular[1], scene->dynamicLights[j].specular[2], scene->dynamicLights[j].specular[3]);
			glUniform1f(scene->renderers[i].shader->loc_lights[j].loc_attenuation, scene->dynamicLights[j].attenuation);
			glUniform1f(scene->renderers[i].shader->loc_lights[j].loc_spotCutoff, scene->dynamicLights[j].spot_cutoff);
			glUniform1f(scene->renderers[i].shader->loc_lights[j].loc_spotCosCutoff, scene->dynamicLights[j].spot_coscutoff);
			glUniform1f(scene->renderers[i].shader->loc_lights[j].loc_spotExponent, scene->dynamicLights[j].spot_exponent);
		}
		for(j=0; j<scene->nLights; j++){
			glUniform4f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_position, scene->lights[j].position.x, scene->lights[j].position.y, scene->lights[j].position.z, scene->lights[j].position.w);
			glUniform4f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_target, scene->lights[j].target.x, scene->lights[j].target.y, scene->lights[j].target.z, scene->lights[j].target.w);
			glUniform4f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_ambient, scene->lights[j].ambient[0], scene->lights[j].ambient[1], scene->lights[j].ambient[2], scene->lights[j].ambient[3]);
			glUniform4f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_diffuse, scene->lights[j].diffuse[0], scene->lights[j].diffuse[1], scene->lights[j].diffuse[2], scene->lights[j].diffuse[3]);
			glUniform4f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_specular, scene->lights[j].specular[0], scene->lights[j].specular[1], scene->lights[j].specular[2], scene->lights[j].specular[3]);
			if(!scene->lights[j].used){
				glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_attenuation, -1.0);
			}else if(scene->lights[j].type != GE_LIGHT_TYPE_SPOT){
				glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_attenuation, -2.0);
			}else{
				glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_attenuation, scene->lights[j].attenuation);
			}
			glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_spotCutoff, scene->lights[j].spot_cutoff);
			glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_spotCosCutoff, scene->lights[j].spot_coscutoff);
			glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_spotExponent, scene->lights[j].spot_exponent);
			glUniform3f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_vector, scene->lights[j].vector.x, scene->lights[j].vector.y, scene->lights[j].vector.z);
			glUniform3f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_targetVector, scene->lights[j].target_vector.x, scene->lights[j].target_vector.y, scene->lights[j].target_vector.z);
			glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_CosInnerMinusOuterAngle, scene->lights[j].CosInnerMinusOuterAngle);
			glUniform1f(scene->renderers[i].shader->loc_lights[j+8/*j+scene->nDynamicLights*/].loc_CosOuterConeAngle, scene->lights[j].CosOuterConeAngle);
		}
		if(ge_current_camera){
			glUniform3f(scene->renderers[i].shader->loc_camera, ge_current_camera->x, ge_current_camera->y, ge_current_camera->z);
		}
		geRenderObjects(&scene->renderers[i]);
		if(scene->renderers[i].callback){
			scene->renderers[i].callback(&scene->renderers[i], -1);
		}
	}
}

void geSceneUpdateMatrices(ge_Scene* scene){
	CalculateModelMatrices();
	/*
	int i = 0;
	for(i=0; i<scene->nRenderers; i++){
		geShaderUse(scene->renderers[i].shader);
		if(ge_current_camera){
			glUniform3f(scene->renderers[i].shader->loc_camera, ge_current_camera->x, ge_current_camera->y, ge_current_camera->z);
		}
		geUpdateMatrix();
		geShaderUse(NULL);
	}
	*/

}

