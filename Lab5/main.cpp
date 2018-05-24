#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl3.h>
#endif
#include <assert.h>
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "obj_parser.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --------------------------------------------------------------------------------------------------- //
// ------------------------------------------ MACROS ------------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// TEXTURE ARRAY LOCATIONS
#define ONION_DETERMINED 0
#define ONION_SCARED 1
#define CONCRETE 2
#define RUST 3
#define RUST2 4
#define COPPER 5
#define FROSTED_GLASS 6
#define GRAY 7

// MESH FILE PATHS
#define ONION_BODY_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/onion_body.obj"
#define CARPARK_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/carpark.obj"
#define CAR_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/car.obj"
#define PIPES_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/pipes.obj"
#define SPHERE_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/sphere.obj"
#define ONION_ARM_FP "/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/OBJ Files/onion_arm.obj"

// VAO/TRANSFORMATION MATRIX LOCATIONS
#define ONION_ID 0
#define CARPARK_ID 1
#define CAR_ID 2
#define PIPES_ID 3
#define SPHERE_ID 4
#define ONION_ARM_ID 5

// GAME MODES
#define EASY_MODE 1
#define HARD_MODE 2

#define PI (glm::pi<float>())

using namespace std;


// --------------------------------------------------------------------------------------------------- //
// ------------------------------------------ GLOBAL VARIABLES --------------------------------------- //
// --------------------------------------------------------------------------------------------------- //

const float game_mode = EASY_MODE;

const int num_textures = 9;
const int num_meshes = 6;

// initial positions for the camera, the onion and the light
glm::vec3 camera_position = glm::vec3( -17.064793, -11.556110, -20.854149 );
glm::vec3 onion_position = glm::vec3(-11.846767f,-11.551583f,-20.597113f);
glm::vec3 light_position = glm::vec3(-11.0f, -8.0f, 0.0f);

// speed at which the camera and onion move
const float speed = 1.0f;

// camera direction and right vectors
glm::vec3 direction;
glm::vec3 right;

// global copy of shader program ID to get a handle on it inside the display function
GLuint globalShaderProgramID;

// Global VAO IDs and mesh data array
GLuint VAO_IDs[num_meshes];
int MESH_POINTS_COUNT[num_meshes];

// transformation matrices for all the meshes
glm::mat4 transformation_matrices[num_meshes];

// texture IDs
GLuint textures[num_textures];

// key states & mouse position
bool keyStates[256];
bool keySpecialStates[256];
int mouse_x, mouse_y;

// onion variables
int onion_expression;
float onion_angle = 0.0f;
int scared_counter = 0;
int hit_count = 0;

// for moving the light sinusoidally
float light_angle = 0.0f;



// --------------------------------------------------------------------------------------------------- //
// ------------------------------------------ FUNCTION DECLARATIONS ---------------------------------- //
// --------------------------------------------------------------------------------------------------- //
// dummy function - won't compile unless a VAO is bound to
void setUpVAOs(GLuint IDs);

// viewing functions
glm::mat4 computeViewMatrix();
void moveCamera();

// onion functions
void toggle_onion_expression();
void moveOnion();
void animateOnion();
void stopAnimation();

// gameplay functions
void checkCollision();
void checkGameOver();

// input handling
void processNormalKeys(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void processSpecialKeys(int key, int x, int y);
void keySpecialUp(int key, int x, int y);
void getMousePosition(int x, int y);

// transformations
void rotate(glm::mat4 &transformationMatrix, float angle, glm::vec3 axis);
void scale(glm::mat4 &transformationMatrix, glm::vec3 scalingVector);



// --------------------------------------------------------------------------------------------------- //
// ---------------------------------- TEXTURE, VAO & SHADER FUNCTIONS -------------------------------- //
// --------------------------------------------------------------------------------------------------- //


// Load Texture Function
bool load_texture (const char* file_name, GLuint* tex) {
	int x, y, n;
	int force_channels = 4;
	unsigned char* image_data = stbi_load (file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf (stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
    
	glGenTextures (1, tex);
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, *tex);
	glTexImage2D (
                  GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  x,
                  y,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  image_data
                  );
	glGenerateMipmap (GL_TEXTURE_2D);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
	return true;
}

// Shader Functions
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?
    
    if ( fp == NULL ) { return NULL; }
    
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    
    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    
    fclose(fp);
    
    return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);
    
    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
    
    const char* pShaderSource = readShaderSource( pShaderText);
    
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    GLuint shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }
    
	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "/Users/brianfallon/Desktop/Lab5/Lab5/Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "/Users/brianfallon/Desktop/Lab5/Lab5/Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);
    
    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}
    
	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
    
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// Create VAOs
void generateObjectBuffer(char *mesh_filepath, int vao_index) {
    // load mesh and set up VAO, VBOs
    // pointers that will be pointed to arrays of vertex data loaded from the file
    float* points = NULL;
    float* tex_coords = NULL;
    float* normals = NULL;

    
    // load .obj mesh from file
    // the function allocates memory for an array of points, texture coordinates, and normals
    if (!load_obj_file(mesh_filepath, points, tex_coords, normals, MESH_POINTS_COUNT[vao_index])) {
        fprintf(stderr, "ERROR: loading mesh from file - check location is same as working directory\n");
        exit(1); // put a break-point on this line when testing
    }
    
    //printf("mesh loaded with %i vertex points\n", MESH_POINTS_COUNT[vao_index]);
    
    // create Vertex Array Object for the mesh - mandatory
    glGenVertexArrays(1, &VAO_IDs[vao_index]);
    glBindVertexArray(VAO_IDs[vao_index]);
    
	GLuint positionID = glGetAttribLocation(globalShaderProgramID, "vertex_position");
	GLuint normalID = glGetAttribLocation(globalShaderProgramID, "vertex_normal");
	GLuint textureID = glGetAttribLocation(globalShaderProgramID, "vertex_texture");
    
    //vertices
    unsigned int vp_vbo = 0;
	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, MESH_POINTS_COUNT[vao_index] * 3 * sizeof (float), &points[0], GL_STATIC_DRAW);
    // normals
	unsigned int vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, MESH_POINTS_COUNT[vao_index] * 3 * sizeof (float), &normals[0], GL_STATIC_DRAW);
    // textures
    unsigned int vt_vbo = 0;
    glGenBuffers (1, &vt_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
    glBufferData (GL_ARRAY_BUFFER, MESH_POINTS_COUNT[vao_index] * 2 * sizeof (float), &tex_coords[0], GL_STATIC_DRAW);
	
    // vertices
	glEnableVertexAttribArray (positionID);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (positionID, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    // normals
	glEnableVertexAttribArray (normalID);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (normalID, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    // textures
    glEnableVertexAttribArray (textureID);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
    glVertexAttribPointer (textureID, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    
}

// --------------------------------------------------------------------------------------------- //
// -------------------------------------- DISPLAY, INIT, MAIN ---------------------------------- //
// --------------------------------------------------------------------------------------------- //

void display()
{
    // tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(globalShaderProgramID);
    
    //Declare uniform variables used in the shader
	int model_mat_location = glGetUniformLocation (globalShaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (globalShaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (globalShaderProgramID, "proj");
    
    //Projection matrix : 45 degree Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 projection = glm::perspective(45.5f, 4.0f/3.0f, 0.1f, 100.0f);
	//View Matrix
    glm::mat4 view = computeViewMatrix();
	//Model matrix
	glm::mat4 model = glm::mat4(1.0f);
    
    glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    
    
    
    int glow_location = glGetUniformLocation(globalShaderProgramID, "glow");
    int light_pos_location = glGetUniformLocation(globalShaderProgramID, "light_position_world");
    
    // Update the position of the light
    // light moves side-to-side up and down sinusoidally, more slowly side-to-side
    // numbers are pretty arbitrary
    light_angle += 0.005f*PI;
    if(light_angle == PI)
        light_angle = 0.0f;
    // light moves twice as fast in hard mode relative to easy mode
    light_position[2] = 8.0f + 25.0f*sin(0.1f*game_mode*light_angle);
    light_position[1] = -7.5f + 4.0f*sin(1.0f*game_mode*light_angle);
    // update the light position in the shader
    glUniform3fv(light_pos_location, 1, &light_position[0]);
    
    
    // onion was slightly too big for my liking
    scale(transformation_matrices[ONION_ID], glm::vec3(0.75f));
    
    // update onion's location, then animate
    transformation_matrices[ONION_ID] = translate(glm::mat4(1.0f), onion_position);
    transformation_matrices[ONION_ARM_ID] = transformation_matrices[ONION_ID]*glm::mat4(1.0f);
    moveOnion();
    
    
    /*/Projection matrix : 45 degree Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 projection = glm::perspective(45.5f, 4.0f/3.0f, 0.1f, 100.0f);
	//View Matrix
    glm::mat4 view = computeViewMatrix();
	//Model matrix
	glm::mat4 model = glm::mat4(1.0f);*/
    
    // update uniforms & draw static objects
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    
    // draw carpark
    glBindVertexArray(VAO_IDs[CARPARK_ID]);
    glBindTexture (GL_TEXTURE_2D, textures[CONCRETE]);
	glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[CARPARK_ID]);
    
    // draw car
    glBindVertexArray(VAO_IDs[CAR_ID]);
    glBindTexture (GL_TEXTURE_2D, textures[RUST]);
	glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[CAR_ID]);
    
    // draw another car
    transformation_matrices[CAR_ID] = glm::translate(glm::mat4(1.0f), glm::vec3(0,0,-29));
    model = transformation_matrices[CAR_ID];
    glBindTexture (GL_TEXTURE_2D, textures[RUST2]);
    glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[CAR_ID]);
    
    
    // draw pipes
    model = glm::mat4(1.0f);
    glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(VAO_IDs[PIPES_ID]);
    glBindTexture (GL_TEXTURE_2D, textures[COPPER]);
	glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[PIPES_ID]);
    
    // DRAW MOVING OBJECTS
    
    // update relevant uniforms & draw sphere - shares the same position as the light
    model = translate(transformation_matrices[SPHERE_ID], light_position);
    glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    
    // sphere glows so need to update this uniform
    glUniform1i(glow_location, 1);
    glBindVertexArray(VAO_IDs[SPHERE_ID]);
    glBindTexture(GL_TEXTURE_2D, textures[FROSTED_GLASS]);
    glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[SPHERE_ID]);
    
    // reset glow
    glUniform1i(glow_location, 0);
    
    
    // update model matrix & draw onion
    model = transformation_matrices[ONION_ID];
	glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
    glBindVertexArray(VAO_IDs[ONION_ID]);
    glBindTexture (GL_TEXTURE_2D, onion_expression);
	glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[ONION_ID]);
    
        // update model matrix & draw onion arm
        model = transformation_matrices[ONION_ARM_ID];
        glUniformMatrix4fv (model_mat_location, 1, GL_FALSE, &model[0][0]);
        glBindVertexArray(VAO_IDs[ONION_ARM_ID]);
        glBindTexture (GL_TEXTURE_2D, textures[GRAY]);
        glDrawArrays(GL_TRIANGLES, 0, MESH_POINTS_COUNT[ONION_ARM_ID]);

    
    // check for significant gameplay events
    checkCollision();
    checkGameOver();
    
    glutSwapBuffers();
}

void init()
{
    // dummy function to stop shader errors
    setUpVAOs(*VAO_IDs);
	// Set up the shaders
    globalShaderProgramID = CompileShaders();
    
    // Set up mesh buffer data
	generateObjectBuffer(ONION_BODY_FP, ONION_ID);
    generateObjectBuffer(ONION_ARM_FP, ONION_ARM_ID);
    generateObjectBuffer(CARPARK_FP, CARPARK_ID);
    generateObjectBuffer(CAR_FP, CAR_ID);
    generateObjectBuffer(PIPES_FP, PIPES_ID);
    generateObjectBuffer(SPHERE_FP, SPHERE_ID);


    // load textures
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/onion_determined.jpg", &textures[ONION_DETERMINED]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/onion_scared.jpg", &textures[ONION_SCARED]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/concrete.jpg", &textures[CONCRETE]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/rust.jpg", &textures[RUST]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/rust2.jpg", &textures[RUST2]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/copper.jpg", &textures[COPPER]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/frosted_glass.jpg", &textures[FROSTED_GLASS]));
    assert(load_texture ("/Users/brianfallon/Desktop/4th Year/Michelmas/CS4052/Labs/Lab 5/Textures/gray.jpg", &textures[GRAY]));
    onion_expression = int(textures[ONION_DETERMINED]);
    
}

int main(int argc, char** argv){
    
	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE|GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Onion");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(display);
    
    //input handling functions
	glutKeyboardFunc(processNormalKeys);
    glutKeyboardUpFunc(keyUp);
	glutSpecialFunc(processSpecialKeys);
    glutSpecialUpFunc(keySpecialUp);
    glutPassiveMotionFunc(getMousePosition);
    
    // set up VAOs and shaders
	init();
    
    
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}

// --------------------------------------------------------------------------------------------- //
// -------------------------------------- FUNCTION IMPLEMENTATIONS ----------------------------- //
// --------------------------------------------------------------------------------------------- //

void setUpVAOs(GLuint IDs)
{
    glGenVertexArrays(1, &IDs);
    glBindVertexArray(IDs);
}

// Viewing Functions
glm::mat4 computeViewMatrix()
{
    float horizontalAngle = 0.0f;
    float verticalAngle = 0.0f;
    float mouseSpeed = 0.005f;
    
    if(mouse_x >=300)
        mouse_x = 300;
    if(mouse_x <= -300)
        mouse_x = -300;
    horizontalAngle += mouseSpeed * float(800/2 - mouse_x );
    //verticalAngle += mouseSpeed * float(600/2 - mouse_y );
    //verticalAngle = 0.0f;
    
    direction = glm::vec3(cos(verticalAngle) * sin(horizontalAngle),
                          sin(verticalAngle),
                          cos(verticalAngle) * cos(horizontalAngle));
    
    // movement is always horizontal, right vector is constant
    //right = glm::vec3(-cos(horizontalAngle), 0, sin(horizontalAngle));
    right = glm::vec3(0, 0, 1);
    
    glm::vec3 up = glm::cross(right, direction);
    
    glm::mat4 viewMatrix = glm::lookAt(camera_position, camera_position+direction, up);
    return viewMatrix;
    
}

void moveCamera()
{
    // move the camera according to keyboard input - unused for gameplay
    if(keySpecialStates[GLUT_KEY_UP])
    {
        camera_position += direction*0.03f*speed;
    }
    if(keySpecialStates[GLUT_KEY_DOWN])
    {
        camera_position -= direction*0.03f*speed;
    }
    if(keySpecialStates[GLUT_KEY_RIGHT])
    {
        camera_position += right*0.03f*speed;
    }
    if(keySpecialStates[GLUT_KEY_LEFT])
    {
        camera_position -= right*0.03f*speed;
    }
}

// Onion Functions
void moveOnion()
{
    // move the camera and onion right/left
    // stop the animation nicely when movement stops
    if(keySpecialStates[GLUT_KEY_RIGHT])
    {
        onion_position[2] += 0.03f*speed;
        camera_position += right*0.03f*speed;
        animateOnion();
    }
    else if(keySpecialStates[GLUT_KEY_LEFT])
    {
        onion_position[2] -= 0.03f*speed;
        camera_position -= right*0.03f*speed;
        animateOnion();
    }
    else stopAnimation();
}

void animateOnion()
{
    // wobble the onion back and forth as it moves
    onion_angle += 0.005f*PI;
    // reset the angle to zero after a full cycle
    if(onion_angle >= PI)
        onion_angle = 0.0f;
    
    // rotate the onion
    rotate(transformation_matrices[ONION_ID],
           0.25f*sin(4.0f*onion_angle), glm::vec3(1,0,0));
    
    // premultiply by the onion's transformation matrix
    transformation_matrices[ONION_ARM_ID] = rotate(glm::mat4(1.0f),
                                                   0.8f*sin(2*onion_angle),
                                                   glm::vec3(1,0,0));
    transformation_matrices[ONION_ARM_ID] =
    transformation_matrices[ONION_ID]*transformation_matrices[ONION_ARM_ID];
}

void stopAnimation()
{
    // makes the onion quiver for a little while after an arrow key has been released for a more natural feel to the movement
    // does nothing if the onion is static

    // decrease the angle if positive, increase if negative
    //aiming for zero to stop the animation
    if(onion_angle > 0)
        onion_angle -= 0.005f*PI;
    if(onion_angle < 0)
        onion_angle += 0.005f*PI;
    if(onion_angle != 0)
    {
        // multiply the sin by onion_angle, so the magnitude
        // of the quivering decreases along with the frequency
        rotate(transformation_matrices[ONION_ID],
               0.1f*onion_angle*sin(4.0f*onion_angle),
               glm::vec3(1,0,0));
    }
}

void toggle_onion_expression()
{
    if(onion_expression == textures[ONION_DETERMINED])
        onion_expression = textures[ONION_SCARED];
    else onion_expression = textures[ONION_DETERMINED];
}

// Gameplay Functions

void checkCollision()
{
    // if the distance between the onion and the light/sphere is less than a threshold
    // increment the hit counter and make the onion scared for a second or so
    
    float ydistance = pow(light_position[1] - onion_position[1],2);
    float zdistance = pow(light_position[2] - onion_position[2],2);
    float distance = sqrt(ydistance + zdistance);
    
    // threshold increases with level difficulty
    //light has to be twice as close to hit you in easy mode than in hard mode
    
    if(distance < (4.0f*game_mode) && scared_counter == 0)
    {
        // trigger the scared expression for 500 frames
        onion_expression = textures[ONION_SCARED];
        scared_counter  = 500;
        hit_count++;
    }
    else if(scared_counter  != 0)
    {
        // decrement the counter if it's not zero
        scared_counter = scared_counter  -1;
    }
    else
    {
        // return to original expression once 500 frames is up
        onion_expression = textures[ONION_DETERMINED];
    }
}

void checkGameOver()
{
    // if the onion goes past a certain point
    //calculate score (decreases with hit count)
    //print to console and exit
    if(onion_position[2] > 46)
    {
        double score = 100*pow(0.5,hit_count);
        printf("Your Score: ");
        printf("%.2f\n", score);
        exit(0);
    }
}


// -------------------------------------------------------------------------------------------- //
// -------------------------------------- INPUT HANDLING -------------------------------------- //
// -------------------------------------------------------------------------------------------- //

// gets mouse position
void getMousePosition(int x, int y)
{
    mouse_x = x;
    mouse_y = y;
}

// detects standard key pressed
void  processNormalKeys(unsigned char key, int x, int y)
{
    keyStates[key] = true;
	switch(key){
        case 'f':
            glutFullScreen();
            break;
        case 27:
            exit(0);
            break;
	}
    
}

// detects standard key released
void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = false;
}

// detects special key pressed
void processSpecialKeys(int key, int x, int y)
{
    keySpecialStates[key] = true;
}

// detects special key released
void keySpecialUp(int key, int x, int y)
{
    keySpecialStates[key] = false;
    
}


// --------------------------------------------------------------------------------------------- //
// -------------------------------------- TRANSFORMATIONS -------------------------------------- //
// --------------------------------------------------------------------------------------------- //

// avoids repetition of clunky rotation code
void rotate(glm::mat4 &transformationMatrix, float angle, glm::vec3 axis)
{
	//temp vector to store displacement of triangle from origin for rotation purposes
	glm::vec3 pivot(transformationMatrix[0][3], transformationMatrix[1][3], transformationMatrix[2][3]);
	//translate to the origin
	transformationMatrix[0][3] = transformationMatrix[1][3] = transformationMatrix[2][3] = 0.0f;
	//rotate
	transformationMatrix = glm::rotate(transformationMatrix, angle, axis);
	//translate back
	transformationMatrix[0][3] = pivot[0];
	transformationMatrix[1][3] = pivot[1];
	transformationMatrix[2][3] = pivot[2];
}

// avoids repetition of clunky scaling code
void scale(glm::mat4 &transformationMatrix, glm::vec3 scalingVector)
{
	//temp vector to store displacement of triangle from origin for scaling purposes
	glm::vec3 pivot(transformationMatrix[0][3], transformationMatrix[1][3], transformationMatrix[2][3]);
	//translate to the origin
	transformationMatrix[0][3] = transformationMatrix[1][3] = transformationMatrix[2][3] = 0.0f;
	//scale
	transformationMatrix = glm::scale(transformationMatrix, scalingVector);
	//translate back
	transformationMatrix[0][3] = pivot[0];
	transformationMatrix[1][3] = pivot[1];
	transformationMatrix[2][3] = pivot[2];
    
    
}




