//
// Created by kovi on 2/19/17.
//

#ifndef SHADOW_MTLREADER_HPP
#define SHADOW_MTLREADER_HPP

#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include "TGALoader.hpp"

struct Material {
    bool containsNs = false, containsNi = false, containsd = false, containsTr = false, containsillum = false;
    std::string name;
    float Ns;
    float Ni;
    float d;
    float Tr;
    NS_TGALOADER::IMAGE textureImage;
    unsigned int textID;

    std::vector<float> Tf;
    int illum;
    std::vector<float> Ka, Kd, Ks, Ke;
    std::string map_Ka, map_Kd, map_refl;
    std::string map_bump, bump;

    bool hasTexture() {
        return map_Kd.size() > 0;
    }

    void setOGL() {
        if (hasTexture()) {
            glEnable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            glBindTexture(GL_TEXTURE_2D, textID);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
    }
};

std::ostream &operator<<(std::ostream &os, const Material &material) {
    std::ostream &formattedOs = os << std::fixed << std::setprecision(4);
    formattedOs << "newmtl " << material.name << std::endl;
    if (material.containsNi)
        formattedOs << "\tNs " << material.Ns << std::endl;
    if (material.containsNs)
        formattedOs << "\tNi " << material.Ni << std::endl;
    if (material.containsd)
        formattedOs << "\td " << material.d << std::endl;
    if (material.containsTr)
        formattedOs << "\tTr " << material.Tr << std::endl;
    if (material.Tf.size() > 0)
        formattedOs << "\tTf " << material.Tf[0] << " " << material.Tf[1] << " " << material.Tf[2] << " " << std::endl;
    if (material.containsillum)
        os << "\tillum " << material.illum << std::endl;
    if (material.Ka.size() > 0)
        formattedOs << "\tKa " << material.Ka[0] << " " << material.Ka[1] << " " << material.Ka[2] << std::endl;
    if (material.Kd.size() > 0)
        formattedOs << "\tKd " << material.Kd[0] << " " << material.Kd[1] << " " << material.Kd[2] << std::endl;
    if (material.Ks.size() > 0)
        formattedOs << "\tKs " << material.Ks[0] << " " << material.Ks[1] << " " << material.Ks[2] << std::endl;
    if (material.Ke.size() > 0)
        formattedOs << "\tKe " << material.Ke[0] << " " << material.Ke[1] << " " << material.Ke[2] << std::endl;
    if (material.map_Ka.length() > 0)
        os << "\tmap_Ka " << material.map_Ka << std::endl;
    if (material.map_Kd.length() > 0)
        os << "\tmap_Kd " << material.map_Kd << std::endl;
    if (material.map_refl.length() > 0)
        os << "\tmap_refl " << material.map_refl << std::endl;
    if (material.map_bump.length() > 0)
        os << "\tmap_bump " << material.map_bump << std::endl;
    if (material.bump.length() > 0)
        os << "\tbump " << material.bump << std::endl;
    return os;
}

bool startsWith(const std::string &line, const std::string &prefix) {
    return line.find(prefix, 0) == 0;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

template<typename T>
std::vector<T>
getSplittedData(const std::string &delimitedData, std::function<T(const std::string &)> converter, const std::string delimiter = " ") {
    std::string workingCopy = delimitedData;
    trim(workingCopy);
    std::vector<T> triple;

    for (int i = 0; workingCopy.length() > 0; ++i) {
        std::string numberString = workingCopy.substr(0, workingCopy.find(delimiter));
        triple.push_back(converter(numberString));
        workingCopy.erase(0, numberString.length() + delimiter.length());
    }

    return triple;
}

float stringToFloat(const std::string &s) {
    return std::stof(s);
}

std::vector<Material> loadMaterialsFromFile(const char *fileName) {
    FILE *f;
    char *line = NULL;
    size_t lineLength = 0, l;
    int lines = 0;
    std::vector<Material> loadedMaterials;

    if (NULL == (f = fopen(fileName, "rt")))
        throw fileName;

    Material material = Material();
    bool firstNewMTL = true;

    while (-1 != (l = ::getline(&line, &lineLength, f))) {
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
        else if (startsWith(workline, "newmtl")) {
            if (!firstNewMTL) {
                loadedMaterials.push_back(material);
                material = Material();
                material.name = workline.substr(workline.find(" ", 0) + 1, std::string::npos);
            } else {
                material.name = workline.substr(workline.find(" ", 0) + 1, std::string::npos);
                firstNewMTL = false;
            }
        }
        else if (startsWith(workline, "Ns")) {
            material.Ns = std::stof(workline.substr(3));
            material.containsNs = true;
        } else if (startsWith(workline, "Ni")) {
            material.Ni = std::stof(workline.substr(3));
            material.containsNi = true;
        } else if (startsWith(workline, "d")) {
            material.d = std::stof(workline.substr(2));
            material.containsd = true;
        } else if (startsWith(workline, "Tr")) {
            material.Tr = std::stof(workline.substr(3));
            material.containsTr = true;
        } else if (startsWith(workline, "Tf")) {
            workline.erase(0, 3);
            material.Tf = getSplittedData<float>(workline, stringToFloat);
        } else if (startsWith(workline, "illum")) {
            material.illum = std::stoi(workline.substr(6));
            material.containsillum = true;
        } else if (startsWith(workline, "Ka")) {
            workline.erase(0, 3);
            material.Ka = getSplittedData<float>(workline, stringToFloat);
        } else if (startsWith(workline, "Kd")) {
            workline.erase(0, 3);
            material.Kd = getSplittedData<float>(workline, stringToFloat);
        } else if (startsWith(workline, "Ks")) {
            workline.erase(0, 3);
            material.Ks = getSplittedData<float>(workline, stringToFloat);
        } else if (startsWith(workline, "Ke")) {
            workline.erase(0, 3);
            material.Ke = getSplittedData<float>(workline, stringToFloat);
        } else if (startsWith(workline, "map_Ka")) {
            material.map_Ka = workline.substr(7);
        } else if (startsWith(workline, "map_Kd")) {
            material.map_Kd = workline.substr(7);
            bool success = material.textureImage.LoadTGA(material.map_Kd.c_str());
            glGenTextures(1, &(material.textID));
            glBindTexture(GL_TEXTURE_2D, material.textID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, material.textureImage.getWidth(), material.textureImage.getHeight(), 0, GL_RGB,
                         GL_UNSIGNED_BYTE, material.textureImage.getDataForOpenGL());
            std::cout << material.map_Kd << " " << success << " " << material.textureImage.getWidth() << " " << material.textureImage.getHeight() << std::endl;
        } else if (startsWith(workline, "map_refl")) {
            material.map_refl = workline.substr(9);
        } else if (startsWith(workline, "map_bump")) {
            material.map_bump = workline.substr(9);
        } else if (startsWith(workline, "bump")) {
            material.bump = workline.substr(5);
        } else {
            std::cout << workline.length() << " Error in this line: |" << workline << "|" << std::endl;
        }
    }
    loadedMaterials.push_back(material);
    free(line);
    fclose(f);

    return loadedMaterials;
};

#endif //SHADOW_MTLREADER_HPP
