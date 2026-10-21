// O que cada botão faz: data-acao no botão é o nome da ação e data-valor, o argumento.
import { aoVivo, set, simulando, state, vagas } from './estado.js';

function marcarSimulada(id, status) {
  return state.vagasSimuladas.map(v => (v.id === id ? { ...v, status } : v));
}

function escolher(id) {
  const vaga = vagas().find(v => v.id === id);
  if (!vaga) return;
  if (vaga.status !== 'free') return set({ notice: vaga.status === 'offline' ? `${id}: ${aoVivo() ? 'sensor sem leitura' : 'sem conexão com o ESP32'}. Disponibilidade desconhecida.` : `${id}: esta vaga já está ocupada.` });
  set({ selected: id, notice: '' });
}

export const acoes = {
  comecar: () => set({ screen: 'map' }),
  voltar: () => (state.screen === 'parked'
    ? set({ notice: `Toque em “${simulando() ? 'Sair e liberar vaga' : 'Sair da vaga'}” quando for embora.` })
    : set({ screen: state.screen === 'map' ? 'welcome' : 'map', notice: '' })),
  abrirSobre: () => set({ info: true }),
  fecharSobre: () => set({ info: false }),
  filtrar: () => set({ onlyFree: !state.onlyFree }),
  setor: zone => set({ zone, notice: '' }),
  vaga: id => escolher(id),
  irParaVaga: () => set({ screen: 'assist', stage: 0, notice: '' }),
  simular: i => set({ stage: Number(i) }),
  confirmar: () => set({ screen: 'parked', notice: '', vagasSimuladas: simulando() ? marcarSimulada(state.selected, 'occupied') : state.vagasSimuladas }),
  sair: () => set({ screen: 'map', selected: null, stage: 0, notice: '', vagasSimuladas: simulando() ? marcarSimulada(state.selected, 'free') : state.vagasSimuladas }),
  fecharAviso: () => set({ notice: '' }),
};
