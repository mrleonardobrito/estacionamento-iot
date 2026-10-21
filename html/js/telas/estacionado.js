// Confirmação de que o carro ficou estacionado.
import { simulando, state } from '../estado.js';
import { icone } from '../icones.js';
import { setorDe } from '../regras.js';
import { aviso, cabecalho } from './partes.js';

export function telaEstacionado() {
  return `<main class="page container parked">
      ${cabecalho('ESTACIONAMENTO IFAL', 'Tudo certo por aqui')}
      <div class="success">
        <div class="success-icon">${icone('ok')}</div>
        <span class="eyebrow">MANOBRA CONCLUÍDA</span>
        <h2>Estacionou.<br>Pode ficar tranquilo.</h2>
        <p>Sua vaga ficou salva${simulando() ? ' nesta simulação' : ''}.</p>
        <div class="saved-spot"><strong>${state.selected}</strong><span>Setor ${setorDe(state.selected)} · Térreo<br>Estacionamento IFAL</span>${icone('ok')}</div>
        <div class="showroom success-car"><img src="car-hero.webp" alt="Carro vermelho estacionado"></div>
        <div class="action-bar">${aviso()}<button class="primary" data-acao="sair">${simulando() ? 'Sair e liberar vaga' : 'Sair da vaga'} ${icone('sair')}</button></div>
      </div>
    </main>`;
}
