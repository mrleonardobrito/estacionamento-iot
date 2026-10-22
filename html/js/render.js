// Monta o HTML da tela e atualiza só o que mudou no DOM. Assim as leituras chegam
// a cada segundo sem perder a rolagem, o foco ou um toque em andamento.
import { state } from './estado.js';
import { telaInicial } from './telas/inicial.js';
import { telaManobra } from './telas/manobra.js';
import { telaMapa } from './telas/mapa.js';
import { topo } from './telas/partes.js';
import { folhaSobre } from './telas/sobre.js';

export const app = document.getElementById('app');
const TELAS = { welcome: telaInicial, map: telaMapa, assist: telaManobra };

export function render(trocouTela) {
  const novo = document.createElement('div');
  novo.innerHTML = topo() + TELAS[state.screen]() + (state.info ? folhaSobre() : '');
  atualizarFilhos(app, novo);
  document.body.style.overflow = state.info ? 'hidden' : '';
  medir();
  if (trocouTela) window.scrollTo(0, 0);
}

// Altura do topo fixo e da barra de ação (no celular ela fica presa embaixo e o conteúdo precisa de espaço).
export function medir() {
  const raiz = document.documentElement.style;
  const barra = app.querySelector('.action-bar');
  raiz.setProperty('--topo', `${app.querySelector('.topbar').offsetHeight}px`);
  raiz.setProperty('--barra', `${barra && getComputedStyle(barra).position === 'fixed' ? barra.offsetHeight : 0}px`);
}

function atualizarFilhos(atual, novo) {
  const antigos = [...atual.childNodes];
  const novos = [...novo.childNodes];
  novos.forEach((filho, i) => (i < antigos.length ? atualizarNo(antigos[i], filho) : atual.appendChild(filho)));
  antigos.slice(novos.length).forEach(no => no.remove());
}

function atualizarNo(atual, novo) {
  if (atual.nodeType !== novo.nodeType || atual.nodeName !== novo.nodeName) return atual.replaceWith(novo);
  if (atual.nodeType === Node.TEXT_NODE) {
    if (atual.nodeValue !== novo.nodeValue) atual.nodeValue = novo.nodeValue;
    return;
  }
  if (atual.nodeType !== Node.ELEMENT_NODE) return;
  for (const { name } of [...atual.attributes]) if (!novo.hasAttribute(name)) atual.removeAttribute(name);
  for (const { name, value } of [...novo.attributes]) if (atual.getAttribute(name) !== value) atual.setAttribute(name, value);
  atualizarFilhos(atual, novo);
}
