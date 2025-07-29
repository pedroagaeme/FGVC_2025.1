# Construção de Pappus na Topologia Circular do Plano Projetivo

Projeto que demonstra a construção geométrica do teorema de Pappus na topologia circular do plano projetivo, usando OpenGL e GLUT para visualização interativa em C++.

## Pré-requisitos

- Sistema Linux (Ubuntu/Debian recomendado)
- Pacotes necessários:

```bash
sudo apt update
sudo apt install build-essential freeglut3 freeglut3-dev libglu1-mesa-dev mesa-common-dev
```

## Compilação

No diretório do projeto, execute:

```bash
g++ src/*.cpp -Iinclude -o app -lGL -lGLU -lglut
```

## Execução

Após compilar, rode:

```bash
./app
```

Uma janela abrirá para visualização e interação com a construção geométrica.

## Controles

- Clique esquerdo: marca pontos no círculo principal.  
- Movimento do mouse: move o ponto atual antes da confirmação.  
- A visualização inclui linhas projetadas ilustrando o teorema de Pappus.