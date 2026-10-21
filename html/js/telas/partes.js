// Partes comuns às telas: topo, cabeçalho com o botão de voltar e aviso.
import { SEM_ESP } from '../config.js';
import { simulando, state } from '../estado.js';
import { icone } from '../icones.js';

export function topo() {
  const [classe, rotulo] = { conectando: ['', 'Conectando…'], online: ['online', 'Ao vivo'], offline: ['offline', 'Sem conexão'] }[state.conexao];
  const detalhe = simulando()
    ? `As vagas e leituras exibidas são simuladas. ${SEM_ESP ? 'Abra a página pelo ESP32 para ver os sensores.' : 'Tentando reconectar…'}`
    : 'As vagas ficam sem sinal até a conexão voltar. Tentando reconectar…';
  const faixa = state.conexao === 'offline'
    ? `<div class="conn-banner" role="status"><div class="container">${icone('info')}<span><strong>Sem conexão com o ESP32.</strong> ${detalhe}</span></div></div>`
    : '';
  return `<header class="topbar">
      <div class="container topbar-inner">
        <div class="wordmark"><span class="brand-mark">P</span>vaga<span class="accent">í</span></div>
        <span class="conn-pill ${classe}"><i></i>${rotulo}</span>
        <button class="icon-button" aria-label="Sobre o vagaí" data-acao="abrirSobre">${icone('info')}</button>
      </div>
      ${faixa}
    </header>`;
}

export function cabecalho(sobretitulo, titulo) {
  return `<div class="page-head"><button class="icon-button" aria-label="Voltar" data-acao="voltar">${icone('voltar')}</button><div><span class="eyebrow">${sobretitulo}</span><h1>${titulo}</h1></div></div>`;
}

export function aviso() {
  return state.notice ? `<div class="notice" role="status">${state.notice}<button aria-label="Fechar aviso" data-acao="fecharAviso">${icone('fechar')}</button></div>` : '';
}
