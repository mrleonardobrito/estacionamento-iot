// Estado da página. Toda mudança passa por set(), que avisa quem desenha a tela.
import { SEM_ESP, TESTE } from './config.js';
import { VAGAS_SIMULADAS } from './simulacao.js';

export const state = {
  screen: 'welcome',                          // welcome | map | assist
  conexao: SEM_ESP ? 'offline' : 'conectando', // conectando | online | offline
  vagasESP: [],                               // { id, zone, status: free | occupied | offline } vindas do ESP32
  vagasSimuladas: TESTE ? VAGAS_SIMULADAS : [], // só no modo de teste
  manobra: null,                              // { vaga, frente } vindo do ESP32
  zone: 'Todos',
  selected: null,
  info: false,
  stage: 0,                                   // leitura simulada escolhida
  onlyFree: false,
  notice: '',
};

export const aoVivo = () => state.conexao === 'online';
// Dados simulados aparecem só no modo de teste, enquanto o ESP32 não responde.
export const simulando = () => TESTE && !aoVivo();

export function vagas() {
  if (simulando()) return state.vagasSimuladas;
  if (aoVivo()) return state.vagasESP;
  return state.vagasESP.map(v => ({ ...v, status: 'offline' }));  // produção sem conexão: estado desconhecido, nunca "livre"
}

// main.js registra aqui a função que desenha a página. Assim o estado não depende das telas.
let aoMudar = () => {};
export const quandoMudar = fn => { aoMudar = fn; };

export function set(patch) {
  const telaAnterior = state.screen;
  const eraAoVivo = aoVivo();
  Object.assign(state, patch);
  // A vaga escolhida deixou de estar livre: outro carro chegou ou os dados trocaram de origem.
  const escolhida = vagas().find(v => v.id === state.selected);
  if (state.screen === 'map' && state.selected && escolhida?.status !== 'free') {
    if (aoVivo() === eraAoVivo) {
      state.notice = escolhida?.status === 'occupied'
        ? `${state.selected}: esta vaga acabou de ser ocupada.`
        : `${state.selected}: ${aoVivo() ? 'sensor sem leitura' : 'sem conexão com o ESP32'}. Disponibilidade desconhecida.`;
    }
    state.selected = null;
  }
  aoMudar(state.screen !== telaAnterior);
}
