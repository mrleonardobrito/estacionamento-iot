// Mapa de vagas, onde o motorista escolhe onde estacionar.
import { state, vagas } from '../estado.js';
import { icone } from '../icones.js';
import { setorDe } from '../regras.js';
import { aviso, cabecalho } from './partes.js';

export function telaMapa() {
  const lista = vagas();
  const livres = lista.filter(v => v.status === 'free').length;
  const sel = state.selected;
  return `<main class="page container map">
      ${cabecalho('VOCÊ ESTÁ NO', 'Estacionamento IFAL')}
      <div class="layout">
        <section class="map-panel" aria-label="Mapa de vagas">${mapa(lista, livres)}</section>
        <aside class="side">
          <div class="action-bar">
            ${aviso()}
            <div class="selection-summary"><div><span class="eyebrow">${sel ? 'VAGA SELECIONADA' : 'SEU PRÓXIMO DESTINO'}</span><strong>${sel ? `${sel} · Setor ${setorDe(sel)}` : 'Escolha onde estacionar'}</strong></div><span class="selection-caption">${sel ? 'Livre agora' : `${livres} opções`}</span></div>
            <button class="primary" ${sel ? '' : 'disabled'} data-acao="irParaVaga">Ir para a vaga ${icone('seta')}</button>
          </div>
          <div class="helper-note">${icone('info')}<p>A vaga selecionada não é uma reserva. A disponibilidade deve ser confirmada ao chegar.</p></div>
        </aside>
      </div>
    </main>`;
}

function mapa(lista, livres) {
  if (!lista.length) return `<div class="helper-note">${icone('info')}<p>${{ conectando: 'Conectando ao ESP32…', online: 'O ESP32 não enviou nenhuma vaga.', offline: 'Sem conexão com o ESP32.' }[state.conexao]}</p></div>`;
  const todas = [...new Set(lista.map(v => v.zone))];
  const zonas = todas.includes(state.zone) ? [state.zone] : todas;
  const variosSetores = todas.length > 1;  // com um setor só, abas e título de setor não acrescentam nada
  const setores = zonas.map(z => {
    const doSetor = lista.filter(v => v.zone === z);
    const livresNoSetor = doSetor.filter(v => v.status === 'free').length;
    return `<section class="parking-sector" aria-label="Setor ${z}">
        ${variosSetores ? `<div class="sector-title"><span>${z}</span><small>SETOR ${z}</small><small>${livresNoSetor} ${livresNoSetor === 1 ? 'livre' : 'livres'}</small></div>` : ''}
        <div class="lot" style="--vagas: ${doSetor.length}">
          <div class="lot-roof"></div><div class="lot-wall"></div><div class="lot-end"></div>
          <div class="lot-pavers"></div><div class="lot-hatch"></div>
          ${doSetor.map(botaoVaga).join('')}
          <div class="lot-lane"><span>${icone('seta')} ENTRADA</span><span>SAÍDA ${icone('seta')}</span></div>
          <div class="lot-sidewalk"></div>
        </div>
      </section>`;
  }).join('');
  return `<div class="availability"><span><strong>${livres}</strong> vagas livres <span class="muted">/ ${lista.length}</span></span><button class="filter-button ${state.onlyFree ? 'active' : ''}" aria-pressed="${state.onlyFree}" data-acao="filtrar">${state.onlyFree ? 'Ver todas' : 'Só livres'}</button></div>
    ${variosSetores ? `<div class="zone-tabs" aria-label="Setor do estacionamento">${['Todos', ...todas].map(z => `<button aria-pressed="${state.zone === z}" data-acao="setor" data-valor="${z}" class="${state.zone === z ? 'selected' : ''}">${z === 'Todos' ? 'Visão geral' : `Setor ${z}`}</button>`).join('')}</div>` : ''}
    <div class="map-heading"><span>Escolha uma vaga no mapa</span><span class="map-scale">TÉRREO</span></div>
    <div class="parking-map">${setores}</div>
    <div class="legend"><span><i class="free-key"></i>Livre</span><span><i class="occupied-key"></i>Ocupada</span><span><i class="selected-key"></i>Selecionada</span><span><i class="offline-key"></i>Sem sinal</span></div>`;
}

function botaoVaga(vaga) {
  const selecionada = state.selected === vaga.id;
  const apagada = state.onlyFree && vaga.status !== 'free';
  const rotulo = { free: 'Livre', occupied: 'Ocupada', offline: 'Sem sinal' }[vaga.status];
  const miolo = vaga.status === 'occupied'
    ? `<img src="car-top.webp" alt="" class="parked-car color-${(parseInt(vaga.id.replace(/\D/g, ''), 10) || 0) % 4}">`
    : `<span class="slot-state">${selecionada ? icone('ok') : vaga.status === 'offline' ? icone('duvida') : ''}<small>${selecionada ? 'Sua vaga' : rotulo}</small></span>`;
  return `<button class="parking-slot ${vaga.status} ${selecionada ? 'is-selected' : ''} ${apagada ? 'dimmed' : ''}" aria-label="Vaga ${vaga.id}, ${rotulo}${selecionada ? ', selecionada' : ''}" aria-pressed="${selecionada}" data-acao="vaga" data-valor="${vaga.id}"><span class="slot-id">${vaga.id}</span>${miolo}</button>`;
}
