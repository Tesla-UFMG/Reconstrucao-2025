# Reconstrucao-2025

Esse é o software de reconstrução de pista da temporada 2025 da equipe Tesla.

## Dependencias

Todos os passos para a instalação das dependencias aqui são para Linux! Caso você use o Windows, baixe o WSL!

Para instalar o compilador e o make, você pode usar o seguinte comando.

```
apt-get install build-essential
```

Para compilar para Windows para passar o .exe para os colegas, você também pode instalar o mingw32.

```
sudo apt install g++-mingw-w64-x86-64
```

Além disso, para a compilação em Linux, você precisa instalar o SDL2.

```
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev
```

De resto, tudo certo! O SQLite3 e o ImGui já está na pasta do projeto. Apesar de não ser a melhor forma, assim evita dor de cabeça para você!

## Compilação

Para compilar, após ter instalados as dependencias, você precisa selecionar para qual sistema você irá compilar, você deve abrir o Makefile  e modificar a primeira linha.

Para compilar para LINUX, você deve deixar a variavel WINDOWS como falso.

```
WINDOWS := 0
```

Para compilar para WINDOWS, deixe a variável como verdadeira.

```
WINDOWS := 1
```

Para você compilar, você deve usar um dos seguintes comandos.

Compilação padrão: `make`

Compilar e rodar: `make run`

Compilar e checar memory leak: `make check`

Limpar os binários e o executável: `make clean`
