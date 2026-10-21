# vagaí: estacionamento inteligente

Projeto de IoT do IFAL. Cada vaga tem um ESP32 com sensor ultrassônico que detecta se ela está livre ou ocupada. O motorista se conecta ao Wi-Fi da vaga pelo celular e abre uma página que mostra o mapa de vagas e orienta a manobra até a posição ideal.

## Como funciona

```
 ┌──────────────┐   ESP-NOW (estado da vaga)   ┌─────────────┐
 │ car (A01)    │ ───────────────────────────▶ │             │
 │ HC-SR04      │ ◀─────────────────────────── │   central   │
 │ AP vagai-A01 │   ESP-NOW (tabela completa)  │             │
 └──────┬───────┘                              └─────────────┘
        │ Wi-Fi                                       ▲ ▼
        ▼                                     outros cars (A02, B01…)
   celular / PC
   http://192.168.4.1
```

- **car** ([esp32/car/car.ino](esp32/car/car.ino)): um por vaga. Lê o HC-SR04 e define se a vaga está ocupada, com mediana de 5 leituras, histerese (ocupada abaixo de 150 cm, livre acima de 170 cm) e debounce de 3 s. Acende o LED verde ou o vermelho, envia o estado à central a cada segundo e cria a rede `vagai-<ID>`, onde serve a página e a API.
- **central** ([esp32/central/central.ino](esp32/central/central.ino)): recebe o estado de todos os cars e retransmite a tabela completa a cada segundo. Uma vaga que fica 5 s sem dar notícia aparece como `sem_sinal`. A central não serve página.
- **página** ([html/](html/)): HTML, CSS e módulos JavaScript puros, sem bibliotecas. O `gerar_pagina.py` junta tudo num único HTML, que fica gravado no firmware do car, comprimido com gzip.

Se o car fica 5 s sem receber a tabela da central, a página mostra só a vaga dele.

## Estrutura

```
html/
  index.html            esqueleto da página
  css/estilo.css        visual (paleta e componentes do DESIGN.md)
  js/
    main.js             ponto de entrada: cliques, primeira tela e consulta ao ESP32
    config.js           CONFIG (intervalos, limites da manobra) e endereço da API
    estado.js           estado da página e set()
    conexao.js          consulta ao GET /api/estado
    regras.js           setor da vaga e orientação da manobra
    acoes.js            o que cada botão (data-acao) faz
    simulacao.js        vagas e leituras simuladas
    icones.js           ícones SVG
    render.js           monta a tela e atualiza só o que mudou no DOM
    telas/              uma tela por arquivo (inicial, mapa, manobra, estacionado, sobre) e partes comuns
  car-hero.webp         imagens usadas pela página
  car-top.webp
esp32/
  car/car.ino           firmware do nó de vaga
  car/pagina.h          página e imagens embutidas (gerado, não editar)
  central/central.ino   firmware da central
  gerar_pagina.py       gera car/pagina.h a partir de html/
DESIGN.md               paleta, tipografia e efeitos visuais
reference/              imagens de referência do design
```

## Hardware

Por vaga (car):

| Componente  | Pino do ESP32 |
| ----------- | ------------- |
| HC-SR04 TRIG | GPIO 5  |
| HC-SR04 ECHO | GPIO 18 (use divisor de tensão: o ECHO sai em 5 V) |
| LED verde    | GPIO 26 |
| LED vermelho | GPIO 27 |

A central é só um ESP32, sem periféricos.

## Gravando os ESP32

Requisitos: Arduino IDE ou `arduino-cli` com o core **ESP32 3.x**. No core 2.x, mude a assinatura do `onRecv` como indicado no comentário do código.

1. **Central:** grave `esp32/central/central.ino` em um ESP32.
2. **Cars:** para cada vaga, ajuste o topo de `esp32/car/car.ino` e grave:
   ```cpp
   const char* VAGA_ID = "A01";        // até 3 caracteres; a 1ª letra é o setor
   const char* AP_PASS = "estaciona";  // senha da rede, mínimo de 8 caracteres
   ```
   `CHANNEL` precisa ser igual no car e na central (padrão: 1).
3. Conecte o celular na rede `vagai-A01`. A página abre sozinha no portal cativo ou em `http://192.168.4.1`.

Os limites de ocupação (`OCC_ON`, `OCC_OFF`) estão em décimos de cm e devem ser calibrados de acordo com a altura do sensor na maquete.

## Editando a página

Depois de alterar qualquer arquivo em `html/`, gere de novo o cabeçalho embutido no firmware e grave o car:

```sh
python3 esp32/gerar_pagina.py
```

O servidor web do ESP32 atende uma conexão por vez, então o script entrega a página num arquivo só: embute o `css/estilo.css` no `index.html`, junta os módulos de `js/` (a partir do `main.js`), comprime com gzip e grava tudo como arrays `PROGMEM` em `esp32/car/pagina.h`. As imagens continuam em arquivos separados.

Para os módulos poderem ser juntados:

- importe só com `import { a, b } from './arquivo.js';` e exporte com `export` antes de `function`, `const`, `let` ou `class`;
- não repita, entre arquivos, um nome declarado fora de funções: depois de juntados, todos os módulos dividem o mesmo escopo.

Se algo fugir dessas regras, o script para e aponta o arquivo.

### Modo de teste e modo de produção

O modo fica no topo de [html/js/config.js](html/js/config.js):

```js
const MODO = 'teste';  // ou 'producao'
```

|                            | Teste                                                    | Produção                                                                 |
| -------------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------ |
| Dados                      | do ESP32 quando ele responde; senão, vagas e leituras simuladas | só do ESP32                                                       |
| ESP32 sem responder        | mostra as vagas simuladas, com aviso no topo             | as vagas ficam "sem sinal", a manobra fica sem leitura e a página continua tentando |
| Botões de simular distância | sim, na tela de manobra                                 | não                                                                      |

O `gerar_pagina.py` grava a página do ESP32 sempre em produção, seja qual for o valor em `config.js`. Para gravar em modo de teste (por exemplo, para apresentar sem os sensores), use `python3 esp32/gerar_pagina.py --teste`.

### Testando no computador

O navegador não carrega módulos JavaScript de uma página aberta direto do arquivo (`file://`), então sirva a pasta `html/`:

```sh
python3 -m http.server -d html
```

- **Sem ESP32 (modo de teste):** abra `http://localhost:8000`. A página usa vagas e leituras simuladas e mostra o aviso "Sem conexão com o ESP32". Na tela de manobra, há botões para simular distâncias.
- **Com ESP32:** conecte o PC na rede do car e abra `http://localhost:8000/?esp=192.168.4.1`. A página consulta a API desse ESP32 (o car responde com CORS liberado). Para ver exatamente o que o celular vai ver, mude `MODO` para `'producao'`; em teste, se o ESP32 parar de responder, a página volta para os dados simulados.

Os intervalos de consulta e os limites da manobra ficam no objeto `CONFIG`, em [html/js/config.js](html/js/config.js). Hoje a página mostra "Pare" a 20 cm ou menos, "Posição ideal" até 50 cm e "Avance devagar" acima disso.

## API

`GET /api/estado`, servida pelo car e consultada pela página a cada 1 s (a cada 400 ms durante a manobra):

```json
{
  "vagas": [
    { "id": "A01", "estado": "livre" },
    { "id": "A02", "estado": "ocupada" },
    { "id": "B01", "estado": "sem_sinal" }
  ],
  "manobra": { "vaga": "A01", "frente": 40.2 }
}
```

- `estado`: `livre`, `ocupada` ou `sem_sinal`.
- O setor é a primeira letra do `id`.
- `manobra.frente` é a distância do sensor do car, em cm, ou `null` quando não há eco.

## Protocolo ESP-NOW

Os pacotes são enviados em broadcast no canal 1, com versão de protocolo `PROTO = 2`. As structs são `packed` e precisam ser iguais nos dois firmwares.

| Mensagem    | Direção        | Conteúdo |
| ----------- | -------------- | -------- |
| `MSG_HELLO` | car → central  | anúncio na inicialização e a cada 30 s |
| `MSG_DATA`  | car → central  | `seq`, distância (décimos de cm), ocupada, id, MAC |
| `MSG_STATE` | central → cars | tabela `{id, estado}`, em blocos de até 40 entradas |

A central suporta até 64 vagas.
