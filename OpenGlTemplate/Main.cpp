#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "Camera.h"
#include "Shader.h"
#include "Bezier.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
bool intesectionTestSphere(glm::vec3 ray_wor, glm::mat4 viewMatrix, glm::vec3 center, float radius);
glm::vec3 viewportSpaceToWorld(int mouse_x, int mouse_y, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
glm::vec3 intersectionWithZPlane(glm::vec3 dir, glm::vec3 camPos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
glm::mat4 getTextureSpaceCoordsCubic(glm::vec4 b0, glm::vec4 b1, glm::vec4 b2, glm::vec4 b3);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

bool mouseBeingPressed = false;

// lighting
float yLightStartingPos = 1.0f;
glm::vec3 lightPos = glm::vec3(1.0, 1.0f, -1.0f);

//Global matrices

glm::mat4 projection;
glm::mat4 model;

//Cursor

int xCursorPos;
int yCursorPos;

//Bezier

BezierControlPoint selectedBez;
BezierControlPoint targetArray[3];



int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4); //Antialiasing stuff
    

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);

    


    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
   
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); //Antialiasing stuff
    glEnable(GL_PROGRAM_POINT_SIZE);
    

    // build and compile our shader zprogram
    // ------------------------------------
    Shader bezierShader("cubicBezier.vert", "cubicBezierFull.frag");
    Shader planeDrawingShader("general.vert", "transparentPlane.frag");
    Shader pointDrawingShader("general.vert", "points.frag");
   

    float triangleBezier[] = {
        // vertices           // uv
        -1.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f,
         -0.5f, -1.0, 1.0f,  0.0, 0.0f, 0.0f,
         0.0f,  0.5f, 1.0f,  0.0f, 0.0f, 0.0f,
         1.0f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f,
    };

    glm::vec4 b0(-1.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 b1(-0.5f, -1.0f, 1.0f, 1.0f);
    glm::vec4 b2(0.0f, 0.5f, 1.0f, 1.0f);
    glm::vec4 b3(1.0f, 0.0f, 1.0f, 1.0f);

    glm::mat4 uvs = getTextureSpaceCoordsCubic(b0, b1, b2, b3);


    std::cout << uvs[0][0];

    triangleBezier[3] = uvs[0][0];
    triangleBezier[4] = uvs[1][0];
    triangleBezier[5] = uvs[2][0];

    triangleBezier[9] = uvs[0][1];
    triangleBezier[10] = uvs[1][1];
    triangleBezier[11] = uvs[2][1];

    triangleBezier[15] = uvs[0][2];
    triangleBezier[16] = uvs[1][2];
    triangleBezier[17] = uvs[2][2];

    triangleBezier[21] = uvs[0][3];
    triangleBezier[22] = uvs[1][3];
    triangleBezier[23] = uvs[2][3];



    

    unsigned int bezierVBO, bezierVAO;
    glGenVertexArrays(1, &bezierVAO);
    glGenBuffers(1, &bezierVBO);

    glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleBezier), triangleBezier, GL_STATIC_DRAW);

    glBindVertexArray(bezierVAO);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // UV coords attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    float xyPlane[] = {

        5.0f, 5.0f, 0.0f,
        -5.0f, 5.0f, 0.0f,
        -5.0f, -5.0f, 0.0f,

        5.0f, 5.0f, 0.0f,
        -5.0f, -5.0f, 0.0f,
        5.0f, -5.0f, 0.0f,
    };

    unsigned int xyPlaneVBO, xyPlaneVAO;
    glGenVertexArrays(1, &xyPlaneVAO);
    glGenBuffers(1, &xyPlaneVBO);

    glBindBuffer(GL_ARRAY_BUFFER, xyPlaneVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xyPlane), xyPlane, GL_STATIC_DRAW);

    glBindVertexArray(xyPlaneVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    targetArray[0] = BezierControlPoint(&triangleBezier[0]);
    targetArray[1] = BezierControlPoint(&triangleBezier[5]);
    targetArray[2] = BezierControlPoint(&triangleBezier[10]);



    //

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;        
        
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        // view/projection transformations
        projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        //Drawing Bezier

        bezierShader.use();
        bezierShader.setMat4("projection", projection);
        bezierShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        bezierShader.setMat4("model", model);

        //


        //if (mouseBeingPressed) {



        //    glm::vec3 ray = viewportSpaceToWorld(xCursorPos, yCursorPos, projection, camera.GetViewMatrix());


        //    //std::cout << intesectionTestSphere(ray, view, glm::vec3(0,0,0), 1.0f) << std::endl;
        //    
        //    for (int i = 0; i < sizeof(targetArray) / sizeof(BezierControlPoint); i++) {
        //        targetArray[i].modelMatrix = model;

        //        std::cout << targetArray[i].intesectionTestSphere(ray, view) << std::endl;
        //        
        //        if (!targetArray[i].intesectionTestSphere(ray, view)) {
        //            continue;
        //        }
        //        

        //        glm::mat4 viewMinusOne = glm::inverse(camera.GetViewMatrix());
        //        glm::vec3 camPos = glm::vec3(viewMinusOne[3][0], viewMinusOne[3][1], viewMinusOne[3][2]);

        //        targetArray[i].MoveVertexWorld(intersectionWithZPlane(ray, camPos));

        //        glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
        //        glBufferData(GL_ARRAY_BUFFER, sizeof(triangleBezier), triangleBezier, GL_STATIC_DRAW);
        //        break;

        //    }
        //}
        //

        //glBindVertexArray(bezierVAO);
        //glDrawArrays(GL_TRIANGLES, 0, 3);


        //Drawing control points
 
        glBindVertexArray(bezierVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        pointDrawingShader.use();

        pointDrawingShader.setMat4("projection", projection);
        pointDrawingShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        pointDrawingShader.setMat4("model", model);

        glDrawArrays(GL_POINTS, 0, 4);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &bezierVAO);
    glDeleteBuffers(1, &bezierVBO);


    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    xCursorPos = xposIn;
    yCursorPos = yposIn;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    //camera.ProcessMouseMovement(xoffset, yoffset);
    glm::vec3 ray = viewportSpaceToWorld(xposIn, yposIn, projection, glm::inverse(camera.GetViewMatrix()));

    

}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouseBeingPressed = true;
        glm::vec3 ray = viewportSpaceToWorld(xCursorPos, yCursorPos, projection, camera.GetViewMatrix());


        std::cout << intesectionTestSphere(ray, camera.GetViewMatrix(), glm::vec3(0,0,0), 1.0f) << std::endl;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        mouseBeingPressed = false;
    }
        
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

float intersectionTestPlane(glm::vec3 ray_wor, glm::mat4 viewMatrix, glm::vec3 planeNormal, float planeOriginDist) {

    glm::mat4 viewMinusOne = glm::inverse(viewMatrix);
    glm::vec3 O = glm::vec3(viewMinusOne[3][0], viewMinusOne[3][1], viewMinusOne[3][2]);
    float dist = -(glm::dot(planeNormal, O) + planeOriginDist) / glm::dot(ray_wor, planeNormal);

    std::cout << "rayworld1: " << ray_wor[0] << std::endl;
    std::cout << "rayworld2: " << ray_wor[1] << std::endl;
    std::cout << "rayworld3: " << ray_wor[2] << std::endl;
    
    return dist;
}


bool intesectionTestSphere(glm::vec3 ray_wor, glm::mat4 viewMatrix, glm::vec3 center, float radius) {
    //t = -b +-sqrt(b� - c)

    glm::mat4 viewMinusOne = glm::inverse(viewMatrix);
    glm::vec3 camPos = glm::vec3(viewMinusOne[3][0], viewMinusOne[3][1], viewMinusOne[3][2]);

    float b = glm::dot(ray_wor, (camPos - center));
    float c = glm::dot((camPos - center), (camPos - center)) - (radius * radius);

    if (b * b - c <= 0) {
        return false;
    }
    
    return true;
         
}

glm::vec3 viewportSpaceToWorld(int mouse_x, int mouse_y, glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
    //Viewport space to NDC space
    float x = (2.0f * mouse_x) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * mouse_y) / SCR_HEIGHT;
    float z = 1.0f;
    glm::vec3 ray_nds = glm::vec3(x, y, z);


    // NDC space to Homogeneous clip space
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    // Homogeneous clip space to Eye space
    glm::vec4 ray_eye = glm::inverse(projectionMatrix) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0); // telling that it points forward and its not a point

    //Eye space to World space
    glm::vec3 ray_wor = glm::inverse(viewMatrix) * ray_eye;
    ray_wor = glm::normalize(ray_wor);

    return ray_wor;
}

glm::vec3 intersectionWithZPlane(glm::vec3 dir, glm::vec3 camPos) {
    return glm::vec3(camPos[0]-camPos[2]*dir[0]/dir[2], camPos[1] - camPos[2] * dir[1] / dir[2], 0.0f);
}

glm::mat4 getTextureSpaceCoordsCubic(glm::vec4 b0, glm::vec4 b1, glm::vec4 b2, glm::vec4 b3) {
    //bernstein basis matrix
    glm::mat4 M3;

    M3[0][0] = 1.0f;
    M3[0][1] = -3.0f;
    M3[0][2] = 3.0f;            //Column 0
    M3[0][3] = -1.0f;

    M3[1][0] = 0.0f;
    M3[1][1] = 3.0f;
    M3[1][2] = -6.0f;           //Column 1
    M3[1][3] = 3.0f;

    M3[2][0] = 0.0f;
    M3[2][1] = 0.0f;
    M3[2][2] = 3.0f;            //Column 2
    M3[2][3] = -3.0f;

    M3[3][0] = 0.0f;
    M3[3][1] = 0.0f;
    M3[3][2] = 0.0f;            //Column 3
    M3[3][3] = 1.0f;

    
    //    1.0f, 0.0f, 0.0f, 0.0f,
    //    -3.0f, 3.0f, 0.0f, 0.0f,
    //    3.0f, -6.0f, 3.0f, 0.0f,
    //    -1.0f, 3.0f, -3.0f, 1.0f



    //Control points
    glm::mat4x4 Bt(b0.x, b1.x, b2.x, b3.x,
                   b0.y, b1.y, b2.y, b3.y,
                   b0.z, b1.z, b2.z, b3.z,
                   b0.w, b1.w, b2.w, b3.w);


    //glm::vec3 b0(-1.0f, 0.0f, -0.5f);
    //glm::vec3 b1(-0.5f, 0.5f, -0.5f);
    //glm::vec3 b2(0.5f, 0.5f, -0.5f);
    //glm::vec3 b3(1.0f, 0.0f, -0.5f);



    //Control Points in power basis form
    glm::mat4x4 C = M3 * Bt;


    float d0;
    float d1;
    float d2;
    float d3;

    glm::mat3 temp;

    temp[0][0] = C[0][3];
    temp[1][0] = C[1][3];
    temp[2][0] = C[3][3];
    
    temp[0][1] = C[0][2];
    temp[1][1] = C[1][2];
    temp[2][1] = C[3][2];
    
    temp[0][2] = C[0][1];
    temp[1][2] = C[1][1];
    temp[2][2] = C[3][1];

    d0 = glm::determinant(temp);

    temp[0][2] = C[0][0];
    temp[1][2] = C[1][0];
    temp[2][2] = C[3][0];

    d1 = - glm::determinant(temp);

    temp[0][1] = C[0][1];
    temp[1][1] = C[1][1];
    temp[2][1] = C[3][1];

    d2 = glm::determinant(temp);

    temp[0][0] = C[0][2];
    temp[1][0] = C[1][2];
    temp[2][0] = C[3][2];

    d3 = -glm::determinant(temp);

    std::cout << d0 << std::endl;
    std::cout << d1 << std::endl;
    std::cout << d2 << std::endl;
    std::cout << d3 << std::endl;

    float DT1 = d0 * d2 - d1 * d1;
    float DT2 = d1 * d2 - d0 * d3;
    float DT3 = d1 * d3 - d2 * d2;

    float discriminant = 4 * DT1 * DT3 - (DT2 * DT2);

    std::cout << discriminant << std::endl;

    // if serpentine

    

    float tl = 3 * d2 + sqrt(9 * d2 * d2 - 12 * d1 * d3);
    float sl = 6 * d1;


    float tm = 3 * d2 - sqrt(9 * d2 * d2 - 12 * d1 * d3);
    float sm = 6 * d1;

    //glm::vec2 n = glm::normalize(glm::vec2(tm, sm));
    //tm = n.x;
    //sm = n.y;

    float tn = 1.0f;
    float sn = 0.0f;

    glm::mat4 texCoords;

    texCoords[0][0] = tl * tm;
    texCoords[1][0] = tl * tl * tl;
    texCoords[2][0] = tm * tm * tm;
    texCoords[3][0] = 1;

    texCoords[0][1] = (3 * tl * tm - tl * sm - tm * sl)/3;
    texCoords[1][1] = -tl * tl * sl + tl * tl * tl;
    texCoords[2][1] = tm * tm * tm - tm * tm * sm;
    texCoords[3][1] = 1;

    texCoords[0][2] = (3 * tl * tm + sm * sl + 2 * (-tl * sm - tm * sl))/3;
    texCoords[1][2] = tl * sl * sl - 2 * tl * tl * sl + tl * tl * tl;
    texCoords[2][2] = tm * tm * tm - 2 * tm * tm * sm + tm * sm * sm;
    texCoords[3][2] = 1;

    texCoords[0][3] = tl * tm - tm * sl + sm * sl - tl * sm;
    texCoords[1][3] = tl * tl * tl - 3 * tl * tl * sl + 3 * tl * sl * sl - sl * sl * sl;
    texCoords[2][3] = tm * tm * tm - 3 * tm * tm * sm + 3 * tm * sm * sm - sm * sm * sm;
    texCoords[3][3] = 1;

    return texCoords;
}

