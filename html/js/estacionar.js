// Fim da manobra: depois de alguns segundos seguidos na posição ideal, a página volta sozinha ao mapa.
// A ocupação da vaga quem marca é o car (esp32/car/car.ino); aqui só termina a manobra na página.
import { CONFIG } from './config.js';
import { set, simulando, state } from './estado.js';
import { leituraAtual, orientacao } from './regras.js';

let idealDesde = null;  // quando a leitura entrou na área ideal
let relogio = null;     // redesenha a contagem enquanto ela corre

// Chamada a cada mudança de estado, antes de desenhar. Sair da área ideal zera a contagem.
export function acompanharManobra() {
  const naIdeal = state.screen === 'assist' && orientacao(leituraAtual()).estado === 'good';
  if (!naIdeal) {
    idealDesde = null;
    clearInterval(relogio);
    relogio = null;
    return;
  }
  if (idealDesde === null) idealDesde = Date.now();
  if (relogio === null) relogio = setInterval(() => (faltaMs() > 0 ? set({}) : concluir()), 250);
}

const faltaMs = () => Math.max(0, CONFIG.manobra.segundosIdeal * 1000 - (Date.now() - idealDesde));

// null fora da área ideal; senão, os segundos que faltam e quanto da contagem já passou (0 a 1).
export function contagem() {
  if (idealDesde === null) return null;
  const falta = faltaMs();
  return { segundos: Math.ceil(falta / 1000), passou: 1 - falta / (CONFIG.manobra.segundosIdeal * 1000) };
}

function concluir() {
  const vagasSimuladas = simulando()
    ? state.vagasSimuladas.map(v => (v.id === state.selected ? { ...v, status: 'occupied' } : v))
    : state.vagasSimuladas;
  set({ screen: 'map', selected: null, stage: 0, notice: '', vagasSimuladas });
}
