// Tela inicial.
import { icone } from '../icones.js';

export function telaInicial() {
  return `<main class="page container welcome">
      <div class="hero-text">
        <div class="intro-tag">MENOS VOLTAS. MAIS TEMPO.</div>
        <h1>Sua próxima<br>vaga está <span>aqui.</span></h1>
        <p class="intro-copy">Encontre um lugar para estacionar.<br>E a tranquilidade de parar certinho.</p>
      </div>
      <div class="hero showroom"><img src="car-hero.webp" alt="Carro esportivo vermelho visto de frente"></div>
      <div class="hero-cta">
        <div class="welcome-features"><span>${icone('ok')} Vagas no mapa</span><span>${icone('ok')} Manobra assistida</span></div>
        <button class="primary" data-acao="comecar">Encontrar minha vaga ${icone('seta')}</button>
        <p class="demo-caption">Projeto IoT · IFAL <span>Estacionamento inteligente</span></p>
      </div>
    </main>`;
}
