//
// Created by hlahm on 2021-10-12.
//

#ifndef VULKANENGINE2_UTILITIES_H
#define VULKANENGINE2_UTILITIES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct ShaderCode{
    char* code;
    size_t code_size;
};

struct Vertex {
    vec3 position;
    vec3 color;
};

static struct ShaderCode readSPRVFile(const char* filename)
{
    struct ShaderCode shaderCode;

    //opens file to read binary
    FILE *fp;
    fp = fopen( filename, "rb");

    if(fp == NULL){
        fprintf(stderr,"Failed to open SPR-V file\n");
        exit(1);
    }

    //points to the end of the file and gets size.
    fseek(fp, 0l, SEEK_END);
    size_t fileSize = (size_t)ftell(fp);
    rewind(fp);

    //get file content and return it
    char * sprvContent = malloc(fileSize * sizeof(char));
    fread(sprvContent, sizeof(char), fileSize, fp);

    fclose(fp);

    shaderCode.code = sprvContent;
    shaderCode.code_size = fileSize;

    return shaderCode;
}

#endif //VULKANENGINE2_UTILITIES_H
