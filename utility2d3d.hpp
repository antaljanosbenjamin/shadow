#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <algorithm>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <opencv2/opencv.hpp>

#ifndef __linux__
#include "getline.h"
#endif



void drawTextToCenter(cv::Mat &img, const char *text)
{
	int baseline = 0;
	cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_PLAIN, 1.0, 1, &baseline);
	baseline += 1;
	cv::Point textOrg((img.cols - textSize.width) / 2, (img.rows + textSize.height) / 2);
	cv::putText(img, text, textOrg, cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar::all(255), 1);
}



GLuint createTextureFromFile(const char *filename)
{
	GLuint texId;
	cv::Mat cvImage = cv::imread(filename);			// note that it reads every picture as 3x8-bit (BGR).

	if (0 == cvImage.rows*cvImage.cols) throw filename;

	std::vector<GLubyte> uImage;
	uImage.reserve(cvImage.rows*cvImage.cols * 3);

	for (int i = cvImage.rows - 1; i >= 0; i--)
	{
		for (int j = 0; j < cvImage.cols; j++)
		{
			cv::Vec3b c = cvImage.at<cv::Vec3b>(i, j);
			uImage.push_back(c[2]);
			uImage.push_back(c[1]);
			uImage.push_back(c[0]);
		}
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cvImage.cols, cvImage.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, uImage.data());

	return texId;
}



struct PairOrder
{
	bool operator()(const std::pair<int, int>& lhs, const std::pair<int, int>& rhs)
	{
		return lhs.first < rhs.first;
	}
} po;



struct Mesh
{
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> textureUV;
	std::vector<std::vector<int> > vertexIndices;
	std::vector<std::vector<int> > normalIndices;
	std::vector<std::vector<int> > textureIndices;
	std::vector<int> materialIndices;
	int defaultMaterial;
	
	Mesh():defaultMaterial(0){}
	
	GLfloat* getMaterial(int materialIndex)
	{
		static GLfloat colors[][4]=
		{
			{0.0, 1.0, 0.0, 1.0},
			{0.4, 0.4, 0.4, 1.0},
			{0.35, 0.35, 0.35, 1.0},
			{0.2, 0.2, 0.2, 1.0},
			{0.1, 0.1, 0.1, 1.0},
			{1.0, 0.3, 0.3, 1.0},
			{0.1, 0.1, 1.0, 0.5},
			{0.9, 0.2, 0.2, 0.7}
		};
		if (materialIndex >= sizeof(colors)/sizeof(float[4])) materialIndex=0;
		return colors[materialIndex];
	}
	
	void sortByColorIndex()
	{
		std::vector<std::pair<int, int> > orderer;
		for (int i=0; i<materialIndices.size(); i++)
		{
			std::pair<int, int> tmp(materialIndices[i], i);
			orderer.push_back(tmp);
		}
		sort(orderer.begin(), orderer.end(), po);

		std::vector<std::vector<int> > vertexIndices2;
		std::vector<std::vector<int> > normalIndices2;
		std::vector<std::vector<int> > textureIndices2;
		std::vector<int> materialIndices2;

		for (int i=0; i<orderer.size(); i++)
		{
			vertexIndices2.push_back(vertexIndices[orderer[i].second]);
			normalIndices2.push_back(normalIndices[orderer[i].second]);
			textureIndices2.push_back(textureIndices[orderer[i].second]);
			materialIndices2.push_back(materialIndices[orderer[i].second]);
		}

		vertexIndices=vertexIndices2;
		normalIndices=normalIndices2;
		textureIndices=textureIndices2;
		materialIndices=materialIndices2;
	}
	
	int materialStr2MaterialIndex(const char *materialStr)
	{
		static const char z[]="default", a[]="Gray", b[]="DarkGray", c[]="BlackEngine", d[]="Black", e[]="Red", f[]="Window", g[]="RedCrystal";
		static const char *mats[]={z,a,b,c,d,e,f,g};
		int i;
		for (i=sizeof(mats)/sizeof(char*)-1; i>0; --i)
			if (!strncmp(materialStr, mats[i], strlen(mats[i]))) break;
		return i;
	}
	
	void render()
	{
		for (int i=0; i<vertexIndices.size(); i++)
		{
			if (defaultMaterial != materialIndices[i])
			{
				defaultMaterial = materialIndices[i];
				glMaterialfv(GL_FRONT, GL_DIFFUSE, getMaterial(defaultMaterial));
				if (getMaterial(defaultMaterial)[3] != 1.0)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				}
				else
				{
					glDisable(GL_BLEND);
				}
			}
			glBegin(GL_TRIANGLE_FAN);
				for (int j=0; j<vertexIndices[i].size(); j++)
				{
					glNormal3fv(normals.data()+3*normalIndices[i][j]);
					glVertex3fv(vertices.data()+3*vertexIndices[i][j]);
				}
			glEnd();
		}
	}
	
	void renderFixColor()
	{
		for (int i=0; i<vertexIndices.size(); i++)
		{
			glBegin(GL_TRIANGLE_FAN);
				for (int j=0; j<vertexIndices[i].size(); j++)
				{
					glVertex3fv(vertices.data()+3*vertexIndices[i][j]);
				}
			glEnd();
		}
	}
	
	void renderNormalColor()
	{
		for (int i=0; i<vertexIndices.size(); i++)
		{
			glBegin(GL_TRIANGLE_FAN);
				for (int j=0; j<vertexIndices[i].size(); j++)
				{
					glColor3fv(normals.data()+3*normalIndices[i][j]);
					glVertex3fv(vertices.data()+3*vertexIndices[i][j]);
				}
			glEnd();
		}
	}
	
	void read(const char *fileName)
	{
		FILE *f;
		char *line=NULL;
		size_t lineLength=0, l;
		int materialIndex=0;
		int lines=0;
		
		if (NULL == (f=fopen(fileName, "rt"))) return;
		while(-1 != (l=::getline(&line, &lineLength, f)))
		{
			lines++;
			char* workline = line + strspn(line, " \t");
			
			for (l = l-1; l>=0; l--)
			{
				if (workline[l] == 0xd || workline[l] == 0xa) workline[l]=0;
				else break;
			}
			
			if (l<=0 || workline[0] == '#') continue;
			else if (!strncmp(workline, "v ", 2))
			{
				float v[3];
				if (3!=sscanf(workline+2, "%f %f %f", v, v+1, v+2)) printf("Error in line %d: %s", lines, line);
				else {vertices.push_back(v[0]); vertices.push_back(v[1]); vertices.push_back(v[2]);}
			}
			else if (!strncmp(workline, "vt ", 3))
			{
				float vt[2];
				if (2!=sscanf(workline+3, "%f %f", vt, vt+1)) printf("Error in line %d: %s", lines, line);
				else {textureUV.push_back(vt[0]); textureUV.push_back(vt[1]);}
			}
			else if (!strncmp(workline, "vn ", 3))
			{
				float vn[3];
				if (3!=sscanf(workline+3, "%f %f %f", vn, vn+1, vn+2)) printf("Error in line %d: %s", lines, line);
				else {normals.push_back(vn[0]); normals.push_back(vn[1]); normals.push_back(vn[2]);}
			}
			else if (!strncmp(workline, "usemtl ", 7))
			{
				materialIndex = materialStr2MaterialIndex(workline+7);
				if (!materialIndex) printf("Error in line %d: %s", lines, line);
			}
			else if (!strncmp(workline, "f ", 2))
			{
				int vi,ti,ni;
				std::vector<int> vertexI;
				std::vector<int> normalI;
				std::vector<int> textureI;

				workline = workline+strcspn(workline, "0123456789");
				while (*workline)
				{
					if (3!=sscanf(workline, "%d/%d/%d", &vi, &ti, &ni)) printf("Error in line %d: %s", lines, line);
					else
					{
						vi--; ti--; ni--;
						vertexI.push_back(vi);
						textureI.push_back(ti);
						normalI.push_back(ni);
					}
					workline += strcspn(workline, "/")+1;
					workline += strcspn(workline, "/")+1;
					workline += strspn(workline, "0123456789")+1;
				}
				vertexIndices.push_back(vertexI);
				normalIndices.push_back(normalI);
				textureIndices.push_back(textureI);
				materialIndices.push_back(materialIndex);
			}
		}
		free(line);
		fclose(f);
		sortByColorIndex();
	}
};
