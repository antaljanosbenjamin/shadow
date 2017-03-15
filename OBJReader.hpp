//
// Created by kovi on 2/20/17.
//

#ifndef SHADOW_OBJREADER_HPP
#define SHADOW_OBJREADER_HPP

#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "MTLReader.hpp"

class OBJReader {

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> textureUV;
    std::vector<std::vector<int> > vertexIndices;
    std::vector<std::vector<int> > normalIndices;
    std::vector<std::vector<int> > textureIndices;
    std::vector<int> materialIndices;
    std::vector<Material> materials;
    int defaultMaterial = -1;

    void sortByColorIndex() {
        std::vector<std::pair<int, int> > orderer;
        for (int i = 0; i < materialIndices.size(); i++) {
            std::pair<int, int> tmp(materialIndices[i], i);
            orderer.push_back(tmp);
        }
        sort(orderer.begin(), orderer.end(), po);

        std::vector<std::vector<int> > vertexIndices2;
        std::vector<std::vector<int> > normalIndices2;
        std::vector<std::vector<int> > textureIndices2;
        std::vector<int> materialIndices2;

        maxVertices = *std::max_element(vertices.begin(), vertices.end());

        for (int i = 0; i < orderer.size(); i++) {
            vertexIndices2.push_back(vertexIndices[orderer[i].second]);
            normalIndices2.push_back(normalIndices[orderer[i].second]);
            textureIndices2.push_back(textureIndices[orderer[i].second]);
            materialIndices2.push_back(materialIndices[orderer[i].second]);
        }

        vertexIndices = vertexIndices2;
        normalIndices = normalIndices2;
        textureIndices = textureIndices2;
        materialIndices = materialIndices2;
    }

    template<typename T>
    void append(std::vector<T> &target, std::vector<T> source) {
        target.insert(target.end(), source.begin(), source.end());
    }

    int materialNameToIndex(std::string searchedMaterialName) {
        auto position = std::find_if(materials.begin(), materials.end(), [&searchedMaterialName](const Material &mat) {
            return mat.name == searchedMaterialName;
        });
        if (position != materials.end()) {
            return position - materials.begin();
        } else {
            //throw std::invalid_argument("CCCan't find material: " + searchedMaterialName);
            throw "aaa";
        }
    }

public:

    float maxVertices;

    void render() {
        defaultMaterial = -1;
        for (int i = 0; i < vertexIndices.size(); i++) {
            if (defaultMaterial != materialIndices[i]) {
                defaultMaterial = materialIndices[i];
                materials[defaultMaterial].setOGL();
                float dissolve = materials[defaultMaterial].d;
                std::vector<float> matKd = materials[defaultMaterial].Kd;
                std::vector<float> matKa = materials[defaultMaterial].Ka;
                std::vector<float> matKs = materials[defaultMaterial].Ks;
                matKd.push_back(dissolve);
                matKa.push_back(dissolve);
                matKs.push_back(dissolve);
                //std:: cout << defaultMaterial << ". " << materials[defaultMaterial].name << std::endl;
                glMaterialfv(GL_FRONT, GL_DIFFUSE, matKd.data());
                //std::cout << matKd[0] << " " << matKd[1] << " "<< matKd[2] << " "<< matKd[3] << std::endl;
                glMaterialfv(GL_FRONT, GL_AMBIENT, matKa.data());
                //std::cout << matKa[0] << " " << matKa[1] << " "<< matKa[2] << " "<< matKa[3] << std::endl;
                glMaterialfv(GL_FRONT, GL_SPECULAR, matKs.data());
                //std::cout << matKs[0] << " " << matKs[1] << " "<< matKs[2] << " "<< matKs[3] << std::endl;
                //std::cout << "|------------------------------------------------------|" << std::endl;
                if (dissolve != 1.0) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                else {
                    glDisable(GL_BLEND);
                }
            }
            glBegin(GL_TRIANGLE_FAN);
            for (int j = 0; j < vertexIndices[i].size(); j++) {
                glNormal3fv(normals.data() + 3 * normalIndices[i][j]);
                glVertex3fv(vertices.data() + 3 * vertexIndices[i][j]);
                if (materials[defaultMaterial].hasTexture())
                    glTexCoord2fv(textureUV.data() + 2 * textureIndices[i][j]);
            }
            glEnd();
        }
    }

    void read(const char *fileName) {
        FILE *f;
        char *line = NULL;
        size_t lineLength = 0, l;
        int materialIndex = 0;
        int lines = 0;

        if (NULL == (f = fopen(fileName, "rt"))) return;
        while (-1 != (l = ::getline(&line, &lineLength, f))) {
            try {
                lines++;
                std::string workline = line + strspn(line, " \t");

                for (size_t i = 0; i < workline.length();) {
                    if ((workline[i] == 0xa) || (workline[i] == 0xd)) {
                        workline.erase(i, 1);
                    } else {
                        ++i;
                    }
                }

                if (workline.length() <= 0 || workline[0] == '#') continue;
                else if (startsWith(workline, "mtllib ")) {
                    workline.erase(0, 7);
                    materials = loadMaterialsFromFile("media/xwing/x-wing.mtl");
                }
                else if (startsWith(workline, "v  ")) {
                    workline.erase(0, 3);
                    append(vertices, getSplittedData<float>(workline, stringToFloat));
                }
                else if (startsWith(workline, "vt ")) {
                    workline.erase(0, 3);
                    std::vector<float> textCoords = getSplittedData<float>(workline, stringToFloat);
                    textureUV.insert(textureUV.end(), textCoords.begin(), textCoords.end() - 1);
                }
                else if (startsWith(workline, "vn ")) {
                    workline.erase(0, 3);
                    append(normals, getSplittedData<float>(workline, stringToFloat));
                } else if (startsWith(workline, "s ")) {

                } else if (startsWith(workline, "g ")) {

                }
                else if (startsWith(workline, "usemtl ")) {
                    workline.erase(0, 7);
                    try {
                        materialIndex = materialNameToIndex(workline);
                    } catch (std::invalid_argument ia) {
                        std::cout << "Error in line " << lines << ": " << line << std::endl;
                    }
                }
                else if (startsWith(workline, "f ")) {
                    std::vector<int> vertexI;
                    std::vector<int> normalI;
                    std::vector<int> textureI;

                    workline.erase(0, 2);

                    std::vector<std::string> vertexTriples = getSplittedData<std::string>(workline, [](const std::string &s) { return s; },
                                                                                          " ");

                    for (std::string vertexTriple : vertexTriples) {
                        static std::vector<int> indices;
                        indices = getSplittedData<int>(vertexTriple, [](const std::string &s) {
                            if (s.length() == 0)
                                return -1;
                            else
                                return std::stoi(s) - 1;
                        }, "/");
                        vertexI.push_back(indices[0]);
                        textureI.push_back(indices[1]);
                        normalI.push_back(indices[2]);
                    }
                    vertexIndices.push_back(vertexI);
                    normalIndices.push_back(normalI);
                    textureIndices.push_back(textureI);
                    materialIndices.push_back(materialIndex);
                } else {
                    std::cerr << "Errorrrrr with line: " << workline << std::endl;
                }
            } catch (...) {
                std::cerr << "error: |" << line;
            }
        }
        free(line);
        fclose(f);
        sortByColorIndex();

    }
};

#endif //SHADOW_OBJREADER_HPP
