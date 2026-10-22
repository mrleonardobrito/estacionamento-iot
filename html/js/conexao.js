// Comunicação com a central (ESP32).
// GET /api/estado?vaga=A01 (JSON), consultado a cada segundo; vaga = a escolhida no mapa, se houver:
//   {
//     "vagas":   [ { "id": "A01", "setor": "A", "estado": "livre" | "ocupada" | "sem_sinal" } ],  // setor opcional: 1ª letra do id
//     "manobra": { "vaga": "A01", "frente": 40.2 }   // cm; null = sem leitura. manobra null = sem vaga escolhida ou vaga desconhecida
//   }
import { API, CONFIG } from './config.js';
import { set, state } from './estado.js';

const STATUS = { livre: 'free', ocupada: 'occupied', sem_sinal: 'offline' };
const limpar = v => String(v ?? '').replace(/[^\w-]/g, '');
const numero = v => (typeof v === 'number' && Number.isFinite(v) ? v : null);
let ultimaResposta = 0;

export async function consultarESP32() {
  const controle = new AbortController();
  const timer = setTimeout(() => controle.abort(), CONFIG.timeoutMs);
  try {
    const url = state.selected ? `${API}?vaga=${encodeURIComponent(state.selected)}` : API;
    const resposta = await fetch(url, { cache: 'no-store', signal: controle.signal });
    if (!resposta.ok) throw new Error(`HTTP ${resposta.status}`);
    receber(await resposta.json());
    ultimaResposta = Date.now();
  } catch (erro) {
    // Na primeira tentativa, ou depois de um tempo sem resposta, passa a mostrar os dados simulados com aviso.
    const perdeu = state.conexao === 'conectando' || Date.now() - ultimaResposta > CONFIG.semRespostaMs;
    if (perdeu && state.conexao !== 'offline') set({ conexao: 'offline' });
  } finally {
    clearTimeout(timer);
    setTimeout(consultarESP32, state.screen === 'assist' ? CONFIG.intervaloManobraMs : CONFIG.intervaloMs);
  }
}

function receber(dados) {
  if (!Array.isArray(dados.vagas)) throw new Error('Resposta sem a lista "vagas"');
  const vagasESP = dados.vagas.map(v => ({
    id: limpar(v.id),
    zone: limpar(v.setor) || limpar(v.id).charAt(0),
    status: STATUS[v.estado] || 'offline',  // estado desconhecido vira "sem sinal", nunca "livre"
  }));
  const m = dados.manobra;
  const manobra = m && m.vaga ? { vaga: limpar(m.vaga), frente: numero(m.frente) } : null;
  set({ vagasESP, manobra, conexao: 'online' });
}
