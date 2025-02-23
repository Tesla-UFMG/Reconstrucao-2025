# Reconstrucao-2025

Reconstrução de pista da temporada 2025, da equipe Tesla UFMG.

## Dependências

Todos os passos de instalações são para Linux! Caso você use o Windows, baixe o WSL!

Antes de tudo, digite:

```
sudo apt update
```

Agora, instale o compilador e o make com o comando:

```
sudo apt install build-essential
```

Se você quiser compilar para Windows, instale o mingw:

```
sudo apt install mingw-w64
```

Por ultimo, baixe o SDL2 para que a interface rode perfeitamente:

```
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev
```

De resto, tudo certo! O SQLite3 e o ImGui já estão na pasta do projeto.

## Compilação

Para compilar, após ter instalados as dependencias, você precisa selecionar para qual sistema você irá compilar. Para isso, você deve abrir o Makefile  e modificar a primeira linha.

Para compilar para LINUX, você deve deixar a variavel WINDOWS como falso.

```
WINDOWS := 0
```

Para compilar para WINDOWS, deixe a variável como verdadeira.

```
WINDOWS := 1
```

Nosso Makefile possui alguns comandos, que além de compilarem o programa, adiciona algumas funcionalidades, são eles:

Compilação padrão: `make`

Compilar e rodar: `make run`

Compilar e checar memory leak: `make check`

Limpar os binários e o executável: `make clean`

Copia e 'zipa' o executável: make  `make copy`



Se a compilação estiver lenta, use a flag `-j` após o comando desejado para se compilar em paralelo, dessa forma, a compilação irá ocorrer de forma mais rápida.

Ao compilar, será gerado o executável na pasta build. Você pode executar esse arquivo em todos os computadores com o sistema operacional que você selecionou no Makefile. Se você selecionou para compilar para Windows, lembre-se de também copiar os arquivos .dll para o programa executar!

## Executação

Você pode executar o programa digitando na pasta raiz do projeto:

```
make run
```

Mas também é possível executá-lo simplemente abrindo-o como um aplicativo normal. Porém, lembre-se, você só conseguirá abri-lo se você o compilou para o seu sistema operacional.

## Justificativas

### Interface

Para a interface gráfica utilizamos o SDL2, o ImGui e o ImPlot. Selecionamos essas bibliotecas por serem faceis de se usar e por serem bastantes robustas.

O SDL2 é usado principalmente para se fazersimulações gráficas e jogos, por isso, é perfeitamente adequada para o nosso projeto.

Já o ImGui, é simplemente a biblioteca de C++ mais sensacional para se fazer interfaces gráficas. Seu uso é simples e robusto, sendo utilizado em praticamente todos os jogos modernos (Incluindo o GTA 6) como interface de debug/desenvolvimento. Portanto, ao aprender utiliza-lo, abre-se um leque muito grande no mercado.

Apesar do ImGui ser otimo para interfaces, foi levantado a necessidade de se instalar o plugin ImPlot para melhores gráficos.

### Banco de Dados

Para o banco de dados estamos utilizando o SQLite3. O escolhemos por ser uma biblioteca leve, simples, e uma das mais utilizadas em C++ para se mexer com SQL.

## Inteface

A interface foi inspirada nesses projetos:

https://trinacriasimracing.wordpress.com/beginners-guide-to-telemetry-analysis-motec/
https://jpieper.com/tag/diagnostics/


## Datasets

Diante da falta de dados do nosso caro, começamos usando esses datasets.

Drone: https://www.kaggle.com/datasets/kunalkarnik95/drone-flight-video-with-telemetry-gps-esc

Camera: https://www.kaggle.com/datasets/hassanmojab/autvi

Race: https://www.kaggle.com/datasets/alexhexan/fm7-rio-de-janeiro-race-telemetry
