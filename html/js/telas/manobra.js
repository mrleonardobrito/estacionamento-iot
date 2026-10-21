// Manobra assistida: orientação a partir da leitura do sensor da frente.
import { simulando, state } from '../estado.js';
import { icone } from '../icones.js';
import { leituraAtual, orientacao, setorDe } from '../regras.js';
import { LEITURAS_SIMULADAS } from '../simulacao.js';
import { cabecalho } from './partes.js';

export function telaManobra() {
  const leitura = leituraAtual();
  const guia = orientacao(leitura);
  const cm = v => (typeof v === 'number' ? `${Math.round(v)}<span> cm</span>` : '—');
  const simulacao = simulando() ? `<div class="simulation-controls"><span class="eyebrow">SIMULE UMA LEITURA</span><div>${LEITURAS_SIMULADAS.map((l, i) => `<button data-acao="simular" data-valor="${i}" aria-pressed="${state.stage === i}" class="${state.stage === i ? 'active' : ''}">${l.rotulo}</button>`).join('')}</div></div>` : '';
  return `<main class="page container assist">
      ${cabecalho('ESTACIONAMENTO IFAL', 'Vamos estacionar')}
      <div class="layout">
        <div class="assist-diagram ${guia.estado}">
          <div class="front-reading"><small>FRENTE</small><strong>${cm(leitura?.frente)}</strong></div>
          <div class="car-readings"><span class="bay"><img src="car-top.webp" alt="Carro visto de cima dentro da vaga"></span></div>
          <span class="position-caption">${leitura === null ? 'Esta vaga não tem sensores de manobra' : 'Referência de posicionamento'}</span>
        </div>
        <aside class="side">
          <div class="assist-heading"><span class="spot-badge">${state.selected}</span><div><h2>Sua vaga te espera</h2><p>Setor ${setorDe(state.selected)} · Térreo</p></div></div>
          <div class="guidance ${guia.estado}" role="status">${icone('ok')}<h2>${guia.titulo}</h2><p>${guia.mensagem}</p></div>
          ${simulacao}
          <div class="action-bar"><button class="primary" ${guia.podeConfirmar ? '' : 'disabled'} data-acao="confirmar">${guia.podeConfirmar ? 'Confirmar estacionamento' : 'Ajuste a posição para confirmar'}${icone('ok')}</button></div>
          <div class="helper-note">${icone('duvida')}<p>${simulando() ? 'Valores ilustrativos. ' : ''}A orientação depende da posição e calibração dos sensores. Observe sempre o ambiente.</p></div>
        </aside>
      </div>
    </main>`;
}
