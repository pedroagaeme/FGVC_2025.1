Construção de Pappus na Topologia Circular do Plano Projetivo
Este projeto demonstra a construção geométrica do teorema de Pappus dentro da topologia circular do plano projetivo, usando OpenGL e GLUT em C++ para visualização interativa.

🚀 Pré-requisitos
Certifique-se de que seu sistema Linux possui as bibliotecas necessárias:

sudo apt update
sudo apt-get install freeglut3-dev

📂 Estrutura do Projeto
project/
│── include/
│ ├── Vector3.h
│ ├── Matrix3.h
│ ├── utils.h
│ └── graphics.h
│
│── src/
│ ├── Vector3.cpp
│ ├── Matrix3.cpp
│ ├── utils.cpp
│ ├── graphics.cpp
│ └── main.cpp
│
│── README.md

⚙️ Compilação
Navegue até a pasta principal do projeto e execute:

g++ src/*.cpp -Iinclude -o app -lGL -lGLU -lglut

▶️ Execução
Após a compilação, execute:

./app

Isso abrirá uma janela chamada Circle Drawing onde você poderá interagir com os círculos usando o mouse.

🖱️ Controles
Clique Esquerdo: Marca um novo ponto no primeiro círculo (até 3 pontos).

Movimento do Mouse: Move o ponto atual antes de confirmar com clique.

Linhas: Após selecionar dois pontos, uma linha projetada aparece no círculo