// O que cada botão faz: data-acao no botão é o nome da ação e data-valor, o argumento.
import { aoVivo, set, state, vagas } from './estado.js';

function escolher(id) {
  if (state.selected === id) return set({ selected: null, notice: '' });  // tocar de novo desmarca
  const vaga = vagas().find(v => v.id === id);
  if (!vaga) return;
  if (vaga.status !== 'free') return set({ notice: vaga.status === 'offline' ? `${id}: ${aoVivo() ? 'sensor sem leitura' : 'sem conexão com o ESP32'}. Disponibilidade desconhecida.` : `${id}: esta vaga já está ocupada.` });
  set({ selected: id, notice: '' });
}

export const acoes = {
  comecar: () => set({ screen: 'map' }),
  voltar: () => set({ screen: state.screen === 'map' ? 'welcome' : 'map', notice: '' }),
  abrirSobre: () => set({ info: true }),
  fecharSobre: () => set({ info: false }),
  filtrar: () => set({ onlyFree: !state.onlyFree }),
  setor: zone => set({ zone, notice: '' }),
  vaga: id => escolher(id),
  irParaVaga: () => set({ screen: 'assist', stage: 0, notice: '' }),
  simular: i => set({ stage: Number(i) }),
  fecharAviso: () => set({ notice: '' }),
};
