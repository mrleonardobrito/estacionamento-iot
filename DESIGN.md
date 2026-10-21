# 🏎️ Design System & Architectural Guidelines: RIVOX Automobili

## 1. Visão Geral do Concept Design
O conceito visual desse site inspira-se no ambiente urbano sofisticado de estacionamentos subterrâneos de luxo (ref: `reference/02_garagem_estacionamento_subterraneo.png`), na intensidade cromática dos faróis desportivos noturnos (ref: `reference/03_rivox_farois_coming_soon.png`) e na elegância orgânica das tonalidades outonais em pistas molhadas (ref: `reference/04_carro_esportivo_estrada_outono.png`). O resultado é uma interface *Dark Mode Premium*, focada em exclusividade, mistério e alta performance.

---

## 2. Paleta de Cores (Color Palette)
*Baseada no arquivo `01_paleta_cores_estacionamento.png`*

### Cores Primárias & De Fundo
* **Dark Asphalt (Fundo Principal):** `#0C0D10`
  * *Aplicação:* Background geral da página, criando um efeito de garagem/estacionamento escuro e envolvente (ref: `02_garagem_estacionamento_subterraneo.png`).
* **Industrial Concrete (Superfícies & Cards):** `#16181E`
  * *Aplicação:* Content containers, cartões de veículos e secções interativas.
* **Column Charcoal (Bordas & Linhas):** `#262938`
  * *Aplicação:* Linhas de grelha, separadores, bordas e pilares visuais inspirados nas colunas de estacionamento (ref: `02_garagem_estacionamento_subterraneo.png`).

### Cores de Destaque & Ação (Accent Colors)
* **Tail-Light Red (Destaque Principal & Faróis):** `#C53038`
  * *Aplicação:* Botões primários (CTA), texto de destaque, luzes dos faróis interativos e sombras brilhantes (`glow`) (ref: `03_rivox_farois_coming_soon.png`).
* **Deep Burgundy (Gradientes & Sombras):** `#9E2A3B`
  * *Aplicação:* Gradientes de fundo, estados de *hover* e sombras de iluminação traseira (ref: `03_rivox_farois_coming_soon.png`, `04_carro_esportivo_estrada_outono.png`).

### Cores Quentes de Apoio & Contraste
* **Warm Sand (Texto Secundário & Detalhes):** `#DFBF95`
  * *Aplicação:* Subtítulos, ícones de especificações e texto de apoio suave (ref: `04_carro_esportivo_estrada_outono.png`).
* **Terracotta Stripe (Sinalização & Alertas):** `#E1A384`
  * *Aplicação:* Faixas horizontais de demarcação industrial e destaques secundários (ref: `02_garagem_estacionamento_subterraneo.png`).
* **Pure White:** `#FFFFFF`
  * *Aplicação:* Títulos principais de alto contraste (H1, H2).

---

## 3. Tipografia (Typography)

* **Display / Títulos:** `Cinzel`, `Syncopate` ou `Montserrat` (Bold / Black / Uppercase)
  * *Estilo:* Caixas altas, ligeiramente espaçadas (*letter-spacing: 3px-5px*), transmitindo imponência e luxo (ref: `03_rivox_farois_coming_soon.png`).
* **Corpo de Texto / Specs:** `Inter` ou `Space Grotesk` (Regular / Medium)
  * *Estilo:* Excelente legibilidade em fundos escuros, aspeto técnico e moderno.

---

## 4. Efeitos Visuais & Atmosfera (Lighting & UI FX)

1. **Efeito "Car Headlights Glow":**
   * Gradientes radiais focados na parte superior ou nos cartões dos veículos (`radial-gradient(circle, rgba(197, 48, 56, 0.4) 0%, transparent 70%)`) (ref: `03_rivox_farois_coming_soon.png`).
   * Iluminação interativa que responde ao toque ou ao ligar/desligar faróis virtuais.

2. **Demarcação de Estacionamento (Parking Lines):**
   * Linhas finas verticais e horizontais em tom `#262938` e pormenores em `#E1A384` para simular as marcações no chão da garagem (ref: `02_garagem_estacionamento_subterraneo.png`).

3. **Grit & Reflection (Pista Molhada / Outono):**
   * Textura subtil com *backdrop-blur* de `12px` e reflexos metálicos suaves nas bordas dos elementos (`border: 1px solid rgba(255, 255, 255, 0.05)`) (ref: `04_carro_esportivo_estrada_outono.png`).

---

## 5. Mapeamento de Arquivos de Imagem

| Arquivo no Zip | Referência | Função Visual na Landing Page |
| :--- | :--- | :--- |
| `01_paleta_cores_estacionamento.png` | Fonte 1 | Paleta de cores base no inspetor interativo e CSS variáveis[cite: 1] |
| `02_garagem_estacionamento_subterraneo.png` | Fonte 2 | Estética de background escuro, colunas e demarcações[cite: 2] |
| `03_rivox_farois_coming_soon.png` | Fonte 3 | Hero Banner com iluminação de faróis vermelhos e tipografia[cite: 3] |
| `04_carro_esportivo_estrada_outono.png` | Fonte 4 | Card de destaque do acervo com ambientação outonal[cite: 4] |