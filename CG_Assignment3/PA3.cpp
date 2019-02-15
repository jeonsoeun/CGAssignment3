#include<iostream>
#include<stdio.h>
#include<stdint.h>
#include<string>
#include<fstream>
#include<vector>

#include<GL/glew.h>
#include<GL/glut.h>
#include<glm/glm.hpp>
#include<glm/gtx/transform.hpp>
#include<glm/gtc/quaternion.hpp>
#include<glm/gtx/quaternion.hpp>
#define BUILDING_TOTAL_NUM 1 //빌딩의 총개수
using namespace std;

GLfloat g_screenWidth, g_screenHeight; //스크린의 가로 세로 길이.
int g_programID, g_posID, g_colorID; //쉐이더 프로그램 ID, shader의 pos변수 ID, shader의 col변수 ID
struct BuildInfo{
	GLuint color[3]; //건물의 색은 무엇인지.
	GLuint position[3]; // 건물의 위치가 어딘지
	GLuint kind; //무슨 건물인지 
};
class Buildings{
private : 
	static Buildings* instance;
	Buildings(){};
public:
	~Buildings();
	static Buildings* getInstance()
	{
		if (instance == NULL)
			instance = new Buildings();
		return instance;
	}
	vector<GLfloat> building;
	vector<GLfloat> building_index;
	GLuint* VAO;
	GLuint* VBO;
	GLuint* VEO; //array버퍼, vertex버퍼, index버퍼 

	//바닥에 대한 정보.
	GLuint VAO_ground;
	GLuint VBO_ground;
	static GLfloat ground[18];
	//카메라의 x,y,z축.
	GLfloat cameraX;
	GLfloat cameraY;
	GLfloat cameraZ;
	//카메라 위아래 보기.
	GLfloat camPed; 
	GLfloat camPan;

	BuildInfo* building[BUILDING_TOTAL_NUM];

};
Buildings* Buildings::instance;

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
	//create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	GLint Result = GL_FALSE;
	int InfoLogLength;

	//Read the vertex shader code from the file
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if (VertexShaderStream.is_open())
	{
		string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	//Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	//Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	//Read the fragment shader code from the file
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open())
	{
		string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	//Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	//Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	//Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> ProgramErrorMessage(InfoLogLength);
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

void renderScence()
{
	Buildings* instance = Buildings::getInstance();
	//Clear all pixels
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//world coordinate할 행렬 만들고 초기화 및 in변수와 연결
	glm::mat4 worldMat = glm::mat4(0.1f);
	GLuint worldLoc = glGetUniformLocation(g_programID, "worldMat");
	glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &worldMat[0][0]);

	//투영 좌표계로 바꿔줄 행렬 초기화(원근투영으로 초기화.) 및 in변수와 연결. 
	glm::mat4 projMat = glm::mat4(0.1f);
	projMat = glm::perspective(120.0f, g_screenWidth / g_screenHeight, 0.1f, 10.0f);
	GLuint projLoc = glGetUniformLocation(g_programID, "projMat");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMat[0][0]);

	//view coordinate로 바꿔줄 행렬 초기화, 카메라 이동, ped, pan추가, in변수와 연결.
	glm::mat4 viewMat = glm::mat4(1.0f);
	glm::mat4 cameraTrans = glm::translate(glm::vec3(-instance->cameraX, -instance->cameraY, 0.0f));
	viewMat[3] = glm::vec4(0, 0, -instance->cameraZ, 1.0f);
	glm::mat4 pedMat = glm::rotate(-instance->camPed, glm::vec3(1, 0, 0));
	glm::mat4 panMat = glm::rotate(-instance->camPan, glm::vec3(0, 1, 0));
	viewMat = pedMat*panMat*viewMat;
	GLuint viewLoc = glGetUniformLocation(g_programID, "viewMat");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMat[0][0]);

	//바닥그리기
	glBindBuffer(GL_ARRAY_BUFFER, instance->VBO_ground);
	glDisableVertexAttribArray(g_colorID);
	glVertexAttrib3f(g_colorID, 180,180,180);
	glDrawArrays(GL_TRIANGLES, 0, 18);

	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, ((GLvoid*)(0)));
	//for (int i = 0; i < BUILDING_TOTAL_NUM; i++)
	//{
		/*glBindVertexArray(instance->VAO[0]);
		glDisableVertexAttribArray(g_colorID);
		glVertexAttrib3f(g_colorID, 0.1, 0.2, 0.3);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, ((GLvoid*)(0)));*/
	//}
	// 
	//더블 버퍼
	glutSwapBuffers();
}

void init()
{
	Buildings* instance = Buildings::getInstance();
	//initilize the glew and check the errors.
	GLenum res = glewInit();
	if (res != GLEW_OK)
	{
		fprintf(stderr, "Error: '%s' \n", glewGetErrorString(res));
	}
	//select the background color
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClearDepth(1.0);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	instance->cameraX = 0.0;
	instance->cameraY = 0;
	instance->cameraZ = -1.0;
	instance->camPed = 0;
	instance->camPan = 0.0;

	instance->building;
	
}
void myMouse(int button, int state, int x, int y)
{

}
void myKeyboard(unsigned char key, int x, int y)
{
}
void mySpecialKey(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT){}
	else if (key == GLUT_KEY_RIGHT){}
	else if (key == GLUT_KEY_DOWN){}

}

void makeBuildings(GLuint* VAO, GLuint* VBO, GLuint* VEO)
{
	Buildings* instance = Buildings::getInstance();
	instance->building.push_back(1.0);
		1, 0, 1,
		1, 0, -1,
		-1, 0, 1,
		-1, 0, -1,
		1, 2, 1,
		1, 2, -1,
		-1, 2, 1,
		-1, 2, -1,
	
		0, 1, 2, 1, 3, 2,
		4, 0, 2, 4, 2, 6,
		4, 0, 1, 4, 1, 5,
		5, 1, 3, 5, 7, 3,
		7, 3, 6, 6, 2, 3,
		5, 7, 4, 6, 7, 4
	
}

int main(int argc, char**argv){
	//init GLUT and create Window
	//initialize the GLUT
	glutInit(&argc, argv);
	//GLUT_DOUBLE enables double buffering (drawing to a background buffer while the other buffer is displayed)
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	//These two functions are used to define the position and size of the window. 
	glutInitWindowPosition(200, 200);

	g_screenWidth = 800;
	g_screenHeight = 500;
	glutInitWindowSize(g_screenWidth, g_screenHeight);

	//This is used to define the name of the window.
	glutCreateWindow("201411237 전소은- Assignment#1");

	//call initization function
	init();
	Buildings* instance = Buildings::getInstance();//인스턴스를 저장.

	GLuint programID = LoadShaders("VertexShader.txt", "FragmentShader.txt");
	glUseProgram(programID);
	g_programID = programID;

	g_posID = glGetAttribLocation(g_programID, "pos");
	g_colorID = glGetAttribLocation(g_programID, "col");
	
	
	//빌딩 개수만큼 array와 버퍼생성
	instance->VAO = new GLuint[1];
	instance->VBO = new GLuint[1];
	instance->VEO = new GLuint[1];
	
	glGenVertexArrays(1, instance->VAO);
	glGenBuffers(1, instance->VBO);
	glGenBuffers(1, instance->VEO);

	makeBuildings(instance->VAO, instance->VBO, instance->VEO); //buffer에 건물의 점들을 넣음.
	
	//바닥 그릴 buffer생성
	glGenVertexArrays(1, &instance->VBO_ground);
	glBindBuffer(GL_ARRAY_BUFFER, instance->VBO_ground);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 36, &instance->ground, GL_STATIC_DRAW);

	
	glVertexAttribPointer(g_posID, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, ((GLvoid*)(0)));
	glEnableVertexAttribArray(g_posID);

	glutMouseFunc(myMouse);
	glutKeyboardFunc(myKeyboard);
	glutSpecialFunc(mySpecialKey);
	glutDisplayFunc(renderScence);

	//enter GLUT event processing cycle
	glutMainLoop();

	//glDeleteVertexArrays();
	return 1;
}