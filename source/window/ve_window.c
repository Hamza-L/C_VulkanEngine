//
// Created by hlahm on 2021-10-02.
//

#include "ve_window.h"

static struct Ve_window ve_window;

static void initWindow(char* wName, const int width, const int height){

    glfwInit(); //initialize GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //no API
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    ve_window.window = glfwCreateWindow(width, height, wName, NULL, NULL);
    ve_window.WIDTH = width;
    ve_window.HEIGHT = height;
}

