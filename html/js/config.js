// Configuração da página e endereço do ESP32.

// Modo da página: 'teste' ou 'producao'.
//   teste:    enquanto o ESP32 não responde, mostra vagas e leituras simuladas, com um aviso fixo no topo.
//   producao: só dados reais do ESP32. Sem resposta, as vagas ficam "sem sinal" e a página continua tentando.
// O esp32/gerar_pagina.py grava a página do ESP32 sempre em 'producao' (ou em 'teste', com --teste).
const MODO = 'producao';
export const TESTE = MODO === 'teste';

export const CONFIG = {
  intervaloMs: 1000,          // consulta ao ESP32 no mapa e nas outras telas
  intervaloManobraMs: 400,    // consulta mais rápida durante a manobra
  timeoutMs: 2500,            // tempo máximo de espera por uma resposta
  semRespostaMs: 5000,        // depois de conectado, sem resposta por esse tempo = "sem conexão"
  manobra: {                  // limites da manobra assistida (calibre com a maquete; os dois últimos
                              // são também OCC_ON e OCC_OFF em esp32/car/car.ino)
    pareCm: 20,               // frente menor ou igual a isto: "Pare"
    idealMaxCm: 50,           // frente até isto (e acima de pareCm): "Posição ideal"
    chegandoMaxCm: 100,       // frente até isto (e acima de idealMaxCm): "Avance devagar"; acima: "Siga até a vaga"
    segundosIdeal: 5,         // tempo seguido na posição ideal para a manobra terminar sozinha
  },
};

const params = new URLSearchParams(location.search);
const ESP = params.get('esp');
export const API = (ESP ? `http://${ESP}` : '') + '/api/estado';
// Teste aberto do computador (arquivo ou servidor local) sem ?esp=: não há ESP32 para consultar.
export const SEM_ESP = TESTE && !ESP && (location.protocol === 'file:' || ['localhost', '127.0.0.1'].includes(location.hostname));
