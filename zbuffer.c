// Z-Buffer

#include <stdio.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#define INFINITO INT_MAX

void init();
void display();
struct Retangulo;
struct Circulo;
void rasterizaRetangulo(struct Retangulo* retangulo, int* buffer, int telaX, int telaY);
struct Retangulo* criaRetangulo(int px, int py, int width, int height, int z, char r, char g, char b);
void rasterizaCirculo(struct Circulo* circulo, int* buffer, int telaX, int telaY);
struct Circulo* criaCirculo(int px, int py, int raio, int z, char r, char g, char b);
int* zBuffer(int** poligonos_rasterizados, int quantidade_poligonos, int telaX, int telaY, int retornar_buffer_de_cor);

// Definem-se as estruturas para representar polígonos que são,
// posteriormente, rasterizadas para uma array que é, então,
// usada para fazer os cálculos relativos ao algoritmo Z-buffer

// Obs: Temos aqui uma profundidade (propriedade int z) constante pois, 
// por simplicidade, estamos considerando que os polígonos estão
// paralelos ao plano ortogonal ao eixo z, o que, evidentemente, não
// serve para todos os casos mas será suficiente para demonstrarmos
// o funcionamento do algoritmo Z-buffer

struct Retangulo {
    // Posição
    int px;
    int py;

    // Largura
    int width;

    // Altura
    int height;

    // Profundidade - Coordenada Z
    int z;

    // Cor
    char r, g, b;
};

struct Circulo {
    // Posição do centro
    int px;
    int py;

    // Raio
    int raio;

    // Profundidade - Coordenada Z
    int z;

    // Cor
    char r, g, b;
};


int main(int argc, char** argv) {
    glutInit(&argc, argv);                     
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize (512, 512);           
    glutInitWindowPosition (100, 100);                  
    glutCreateWindow ("Z-Buffer");  
    init();
    glutDisplayFunc(display);
    glutMainLoop();   
}

void init() {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glOrtho (0, 512, 0, 512, -1 ,1);
}

void display(){

    // Troque as profundidades e reexecute o programa para ver
    // o funcionamento do algoritmo Z-Buffer

    const int PROFUNDIDADE_CIRCULO_A = 10;
    struct Circulo* circulo_A = criaCirculo(250, 250, 100, PROFUNDIDADE_CIRCULO_A, 10, 150, 50);
    int* circulo_A_rasterizado = malloc(512 * 512 * 4 * sizeof(int));
    rasterizaCirculo(circulo_A, circulo_A_rasterizado, 512, 512);

    const int PROFUNDIDADE_RETANGULO_A = 7;
    struct Retangulo* retangulo_A = criaRetangulo(100, 100, 200, 200, PROFUNDIDADE_RETANGULO_A, 100, 100, 50);
    int* retangulo_A_rasterizado = malloc(512 * 512 * 4 * sizeof(int));
    rasterizaRetangulo(retangulo_A, retangulo_A_rasterizado, 512, 512);

    const int PROFUNDIDADE_CIRCULO_B = 2;
    struct Circulo* circulo_B = criaCirculo(300, 150, 80, PROFUNDIDADE_CIRCULO_B, 250, 250, 250);
    int* circulo_B_rasterizado = malloc(512 * 512 * 4 * sizeof(int));
    rasterizaCirculo(circulo_B, circulo_B_rasterizado, 512, 512);

    const int PROFUNDIDADE_RETANGULO_B = 10;
    struct Retangulo* retangulo_B = criaRetangulo(270, 230, 100, 150, PROFUNDIDADE_RETANGULO_B, 70, 40, 150);
    int* retangulo_B_rasterizado = malloc(512 * 512 * 4 * sizeof(int));
    rasterizaRetangulo(retangulo_B, retangulo_B_rasterizado, 512, 512);

    // Chamada para z-Buffer

    // Coloca os polígonos em um vetor
    int* poligonos[4];
    poligonos[0] = circulo_A_rasterizado;
    poligonos[1] = circulo_B_rasterizado;
    poligonos[2] = retangulo_A_rasterizado;
    poligonos[3] = retangulo_B_rasterizado;

    // Chama o Z-buffer passando os polígonos criados como parâmetro
    int* color_buffer = zBuffer(poligonos, 4, 512, 512, 1);

    // Renderiza o color_buffer, resultado do algoritmo Z-Buffer
    glBegin(GL_POINTS);
    for (int i = 0; i < 512; ++i) {
        for (int j = 0; j < 512; ++j) {
                glColor3ub(color_buffer[(i * 512 + j) * 3], color_buffer[(i * 512 + j) * 3 + 1], color_buffer[(i * 512 + j) * 3 + 2]);
                glVertex2i(i, j); 
        }
    }
    glEnd();
    glFlush();
}

// Z-buffer
int* zBuffer(int** poligonos_rasterizados, int quantidade_poligonos, int telaX, int telaY, int retornar_buffer_de_cor) {
    //  ---------------------------
    // | Implementação Z-Buffer    |
    //  ---------------------------
    int* color_buffer = malloc(512 * 512 * 3 * sizeof(int));
    int* z_buffer = malloc(512 * 512 * sizeof(int));

    // Inicializa todos os elementos de Z_buffer para infinito, que aqui nada
    // mais é que o maior número que um int pode assumir: INT_MAX = 2147483647
    for (int i = 0; i < 512 * 512; i++)
        z_buffer[i] = INFINITO;

    // Inicializa color_buffer com 0's: background preto
    for (int i = 0; i < 512 * 512 * 3; i++)
        color_buffer[i] = 0;

    // Processa cada um dos polígonos passados por parâmetro
    for (int p = 0; p < quantidade_poligonos; p++) {
        for (int i = 0; i < telaX; ++i) {
            for (int j = 0; j < telaY; ++j) {
                if (poligonos_rasterizados[p][(i * telaX + j) * 4] != -1) {
                    int profundidadePixel = poligonos_rasterizados[p][(i * telaX + j) * 4 + 3];
                    int profundidadeZBuffer = z_buffer[i * telaX + j];
                    // Se o pixel atual estiver na frente do pixel previamente pintado em (i, j),
                    // pinta o buffer de cor em (i, j) com a cor do pixel atual,
                    // do contrário, nada é feito
                    if (profundidadePixel < profundidadeZBuffer) {
                        color_buffer[(i * telaX + j) * 3 + 0] = poligonos_rasterizados[p][(i * telaX + j) * 4 + 0];
                        color_buffer[(i * telaX + j) * 3 + 1] = poligonos_rasterizados[p][(i * telaX + j) * 4 + 1];
                        color_buffer[(i * telaX + j) * 3 + 2] = poligonos_rasterizados[p][(i * telaX + j) * 4 + 2];
                        z_buffer[i * telaX + j] = profundidadePixel;
                    }
                }
            }
        }
    }

    return retornar_buffer_de_cor ? color_buffer : z_buffer;
}

// Rasterizadores

// Rasteriza o retângulo descrito pela struct Retangulo. O retângulo é rasterizado 
// para o buffer de tal forma que cada 4 inteiros consecutivos formam um pixel,
// sendo os 3 primeiros para especificar a cor (r, g, b) e o último para especificar
// a profundidade (coordenada z) do pixel 
void rasterizaRetangulo(struct Retangulo* retangulo, int* buffer, int telaX, int telaY) {
    // Varre todo o vetor e define tudo como -1, indicando que os pixels não
    // fazem parte da rasterização do polígono (por enquanto, pelo menos)
    for (int i = 0; i < telaX * telaY * 4; ++i)
        buffer[i] = -1;

    for (int x = retangulo->px; x < retangulo->px + retangulo->width; x++) {
        for (int y = retangulo->py; y < retangulo->py + retangulo->height; y++) {
            
            // Define a cor do pixel
            buffer[(y * telaX + x) * 4 + 0] = (int) retangulo->r;
            buffer[(y * telaX + x) * 4 + 1] = (int) retangulo->g;
            buffer[(y * telaX + x) * 4 + 2] = (int) retangulo->b;

            // Define a profundidade do pixel (que, de acordo com 
            // o que definimos (polígonos paralelos ao plano ortogonal
            // ao eixo z), será sempre a mesma)
            buffer[(y * telaX + x) * 4 + 3] = retangulo->z;
        }
    }
}

// Rasteriza o círculo descrito pela struct Circulo. Analogamente à função rasterizaRetangulo,
// o circulo é rasterizado para o buffer de tal forma que cada 4 inteiros consecutivos formam 
// um pixel, sendo os 3 primeiros para especificar a cor (r, g, b) e o último para especificar
// a profundidade (coordenada z) do pixel 
void rasterizaCirculo(struct Circulo* circulo, int* buffer, int telaX, int telaY) {
    // Varre todo o vetor e define tudo como -1, indicando que os pixels não
    // fazem parte da rasterização do polígono (por enquanto, pelo menos)
    for (int i = 0; i < telaX * telaY * 4; ++i)
        buffer[i] = -1;

    for (int x = 0; x < telaX; x++) {
        for (int y = 0; y < telaY; y++) {
            int distancia = sqrt((x - circulo->px)*(x - circulo->px) + (y - circulo->py)*(y - circulo->py));
            if (distancia < circulo->raio) {
                // Define a cor do pixel
                buffer[(y * telaX + x) * 4 + 0] = (int) circulo->r;
                buffer[(y * telaX + x) * 4 + 1] = (int) circulo->g;
                buffer[(y * telaX + x) * 4 + 2] = (int) circulo->b;

                // Define a profundidade do pixel (que, de acordo com 
                // o que definimos (polígonos paralelos ao plano ortogonal
                // ao eixo z), será sempre a mesma)
                buffer[(y * telaX + x) * 4 + 3] = circulo->z;
            }
        }
    }
}

struct Retangulo* criaRetangulo(int px, int py, int width, int height, int z, char r, char g, char b) {
    struct Retangulo* novoRetangulo = malloc(sizeof(struct Retangulo));
    novoRetangulo->px = px;
    novoRetangulo->py = py;
    novoRetangulo->width = width;
    novoRetangulo->height = height;
    novoRetangulo->z = z;
    novoRetangulo->r = r;
    novoRetangulo->g = g;
    novoRetangulo->b = b;

    return novoRetangulo;
}

struct Circulo* criaCirculo(int px, int py, int raio, int z, char r, char g, char b) {
    struct Circulo* novoCirculo = malloc(sizeof(struct Circulo));
    novoCirculo->px = px;
    novoCirculo->py = py;
    novoCirculo->raio = raio;
    novoCirculo->z = z;
    novoCirculo->r = r;
    novoCirculo->g = g;
    novoCirculo->b = b;

    return novoCirculo;
}

