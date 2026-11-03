// Comunicação com a central (ESP32). vaga = a escolhida no mapa, se houver.
// Canal principal: Server-Sent Events em http://<central>:81/eventos?vaga=A01. A central manda o
// estado assim que ele muda (e pelo menos uma vez por segundo). Enquanto esse canal não entrega
// nada, a página consulta GET /api/estado?vaga=A01 a cada segundo. Os dois usam o mesmo JSON:
//   {
//     "vagas":   [ { "id": "A01", "setor": "A", "estado": "livre" | "ocupada" | "sem_sinal" } ],  // setor opcional: 1ª letra do id
//     "manobra": { "vaga": "A01", "frente": 40.2 }   // cm; null = sem leitura. manobra null = sem vaga escolhida ou vaga desconhecida
//   }
import { API, CONFIG, EVENTOS } from './config.js';
import { set, state } from './estado.js';

const STATUS = { livre: 'free', ocupada: 'occupied', sem_sinal: 'offline' };
const limpar = v => String(v ?? '').replace(/[^\w-]/g, '');
const numero = v => (typeof v === 'number' && Number.isFinite(v) ? v : null);
let ultimaResposta = 0;
let ultimoEvento = 0;
let canal = null;
let vagaOuvida = null;

const comVaga = url => (state.selected ? `${url}?vaga=${encodeURIComponent(state.selected)}` : url);

// Chamada a cada mudança de estado: só (re)abre o canal quando a vaga escolhida muda,
// porque a leitura da manobra que a central envia depende dela.
export function ouvirESP32() {
  if (!window.EventSource || (canal && vagaOuvida === state.selected)) return;  // sem suporte: fica só na consulta
  canal?.close();
  vagaOuvida = state.selected;
  ultimoEvento = 0;  // até o canal novo responder, a consulta cobre
  canal = new EventSource(comVaga(EVENTOS));  // reconecta sozinho se a conexão cair
  canal.onmessage = evento => {
    try {
      receber(JSON.parse(evento.data));
      ultimaResposta = ultimoEvento = Date.now();
    } catch (erro) { /* mensagem inválida: a próxima chega em até 1 s */ }
  };
}

export async function consultarESP32() {
  if (Date.now() - ultimoEvento > CONFIG.eventoValidoMs) await perguntar();
  setTimeout(consultarESP32, state.screen === 'assist' ? CONFIG.intervaloManobraMs : CONFIG.intervaloMs);
}

async function perguntar() {
  const controle = new AbortController();
  const timer = setTimeout(() => controle.abort(), CONFIG.timeoutMs);
  try {
    const resposta = await fetch(comVaga(API), { cache: 'no-store', signal: controle.signal });
    if (!resposta.ok) throw new Error(`HTTP ${resposta.status}`);
    receber(await resposta.json());
    ultimaResposta = Date.now();
  } catch (erro) {
    // Na primeira tentativa, ou depois de um tempo sem resposta, passa a mostrar os dados simulados com aviso.
    const perdeu = state.conexao === 'conectando' || Date.now() - ultimaResposta > CONFIG.semRespostaMs;
    if (perdeu && state.conexao !== 'offline') set({ conexao: 'offline' });
  } finally {
    clearTimeout(timer);
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
