// Ponto de entrada: liga os cliques às ações, desenha a primeira tela e começa a ouvir o ESP32.
import { acoes } from './acoes.js';
import { SEM_ESP } from './config.js';
import { consultarESP32, ouvirESP32 } from './conexao.js';
import { acompanharManobra } from './estacionar.js';
import { quandoMudar } from './estado.js';
import { app, medir, render } from './render.js';

quandoMudar(trocouTela => {
  if (!SEM_ESP) ouvirESP32();  // troca de vaga escolhida reabre o canal de eventos
  acompanharManobra();
  render(trocouTela);
});

app.addEventListener('click', evento => {
  if (evento.target.classList.contains('sheet-overlay')) return acoes.fecharSobre();
  const botao = evento.target.closest('[data-acao]');
  if (botao && !botao.disabled) acoes[botao.dataset.acao](botao.dataset.valor);
});
window.addEventListener('resize', medir);

render(false);
if (!SEM_ESP) { ouvirESP32(); consultarESP32(); }
