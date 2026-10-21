// Folha "Sobre o vagaí", aberta pelo botão de informação do topo.
import { simulando, state } from '../estado.js';
import { icone } from '../icones.js';

export function folhaSobre() {
  const situacao = simulando()
    ? `Sem conexão com o ESP32: ${state.vagasSimuladas.length} vagas e leituras simuladas, só para demonstração.`
    : {
      online: `${state.vagasESP.length} vagas monitoradas pelo ESP32, atualizadas a cada segundo.`,
      conectando: 'Conectando ao ESP32…',
      offline: 'Sem conexão com o ESP32. As vagas voltam a ser atualizadas quando a conexão voltar.',
    }[state.conexao];
  const vendo = `<p>${situacao} Não há reservas nem pagamentos.</p>`;
  return `<div class="sheet-overlay"><div class="sheet-panel" role="dialog" aria-modal="true" aria-label="Sobre o vagaí">
      <div class="sheet-handle"></div>
      <div class="sheet-head"><div><h2>Sobre o vagaí</h2><p>Um projeto de Internet das Coisas.</p></div><button class="icon-button" aria-label="Fechar" data-acao="fecharSobre">${icone('fechar')}</button></div>
      <div class="about"><p>O vagaí ajuda a visualizar vagas e acompanhar o posicionamento do veículo.</p><h3>Como funciona</h3><p>Sensores ultrassônicos medem as distâncias e o ESP32 entrega as leituras a esta página pela rede Wi-Fi.</p><h3>O que você está vendo</h3>${vendo}<p>Para avaliar o alinhamento real, é necessário definir a quantidade, a posição e os limites dos sensores. Um único sensor de distância não verifica todo o posicionamento do carro.</p><button class="primary" data-acao="fecharSobre">Entendi ${icone('ok')}</button></div>
    </div></div>`;
}
