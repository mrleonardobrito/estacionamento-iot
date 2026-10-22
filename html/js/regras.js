// Regras: setor de uma vaga e orientação da manobra a partir da leitura do sensor.
import { CONFIG } from './config.js';
import { aoVivo, simulando, state, vagas } from './estado.js';
import { LEITURAS_SIMULADAS } from './simulacao.js';

export function setorDe(id) {
  return vagas().find(v => v.id === id)?.zone ?? String(id ?? '').charAt(0);
}

export function leituraAtual() {
  if (simulando()) return LEITURAS_SIMULADAS[state.stage];
  if (!aoVivo()) return { vaga: state.selected, frente: null };  // produção sem conexão: leitura desconhecida
  if (!state.manobra || state.manobra.vaga !== state.selected) return null;  // vaga sem sensores de manobra
  return state.manobra;
}

export function orientacao(leitura) {
  const L = CONFIG.manobra;
  if (leitura === null) return { estado: 'adjust', titulo: 'Vaga sem sensores de manobra', mensagem: 'Estacione com atenção e volte ao mapa quando terminar.' };
  if (leitura.frente === null) return { estado: 'danger', titulo: 'Sensor sem leitura', mensagem: aoVivo() ? 'O sensor da frente não respondeu. Observe o ambiente.' : 'Sem conexão com o ESP32. Observe o ambiente.' };
  if (leitura.frente <= L.pareCm) return { estado: 'danger', titulo: 'Pare. Muito próximo!', mensagem: 'A distância frontal está abaixo do limite configurado.' };
  if (leitura.frente > L.chegandoMaxCm) return { estado: 'adjust', titulo: 'Siga até a vaga.', mensagem: 'O carro ainda não está na vaga.' };
  if (leitura.frente > L.idealMaxCm) return { estado: 'adjust', titulo: 'Avance devagar.', mensagem: `Faltam cerca de ${Math.round(leitura.frente - L.idealMaxCm)} cm até a área ideal.` };
  return { estado: 'good', titulo: 'Posição ideal!', mensagem: 'Mantenha o carro parado até a contagem terminar.' };
}
