#!/usr/bin/env python3
"""Gera esp32/central/pagina.h a partir dos arquivos de html/.

O servidor web do ESP32 atende uma conexão por vez, e pedir cada módulo JS
separado deixaria a página lenta para abrir. Por isso o CSS e os módulos
(a partir de js/main.js) são embutidos no index.html antes de comprimir.
As imagens continuam em arquivos separados.

A página do ESP32 sai em modo de produção (só dados reais), seja qual for o
MODO em html/js/config.js. Com --teste, sai em modo de teste (vagas simuladas).

Rode sempre que mudar algo em html/:
    uv run esp32/gerar_pagina.py
"""
import gzip
import re
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
HTML = RAIZ / "html"
SAIDA = RAIZ / "esp32" / "central" / "pagina.h"
MODO = "teste" if "--teste" in sys.argv[1:] else "producao"

ARQUIVOS = [  # (nome no C, arquivo, comprimir com gzip?)
    ("INDEX_HTML_GZ", HTML / "index.html", True),  # com o CSS e o JS embutidos
    ("CAR_HERO_WEBP", HTML / "car-hero.webp", False),  # WebP já vem comprimido
    ("CAR_TOP_WEBP", HTML / "car-top.webp", False),
]

# Únicas formas de import/export aceitas nos módulos.
IMPORT = re.compile(r"^import \{[^}]*\} from '([^']+)';\n", re.MULTILINE)
EXPORT = re.compile(r"^export (?=(?:async )?function |const |let |class )", re.MULTILINE)
DECLARACAO = re.compile(r"^(?:async )?(?:function|const|let|class) ([\w$]+)", re.MULTILINE)


def juntar_modulos(entrada):
    """Junta os módulos num script só, sem import/export.

    Cada módulo entra depois dos que ele importa, na ordem em que o navegador os
    executaria. Como todos passam a dividir o mesmo escopo, um nome declarado no
    nível de cima de um arquivo não pode se repetir em outro.
    """
    vistos, partes, origem = set(), [], {}

    def visitar(arquivo):
        if arquivo in vistos:
            return
        vistos.add(arquivo)
        codigo = arquivo.read_text(encoding="utf-8")
        for dependencia in IMPORT.findall(codigo):
            visitar((arquivo.parent / dependencia).resolve())
        codigo = EXPORT.sub("", IMPORT.sub("", codigo))
        nome = arquivo.relative_to(HTML)
        if re.search(r"^(?:import|export)\b", codigo, re.MULTILINE):
            sys.exit(f"{nome}: use só import {{ ... }} from './arquivo.js'; e export antes de function/const/let/class")
        for declarado in DECLARACAO.findall(codigo):
            if declarado in origem:
                sys.exit(f"{nome}: '{declarado}' também é declarado em {origem[declarado]}; renomeie um dos dois")
            origem[declarado] = nome
        partes.append(f"// ----- {nome} -----\n{codigo}")

    visitar(entrada.resolve())
    return "\n".join(partes)


def definir_modo(js):
    """Troca o MODO de js/config.js pelo escolhido aqui."""
    js, trocas = re.subn(r"^const MODO = '\w+';", f"const MODO = '{MODO}';", js, flags=re.MULTILINE)
    if trocas != 1:
        sys.exit("js/config.js: declare o modo numa linha só, como const MODO = 'teste';")
    return js


def montar_pagina(caminho):
    """Lê a página e embute nela o CSS e os módulos JS locais."""
    pasta = caminho.parent
    pagina = caminho.read_text(encoding="utf-8")
    pagina = re.sub(r'<link rel="stylesheet" href="([^":]+)">',
                    lambda m: f"<style>\n{(pasta / m[1]).read_text(encoding='utf-8')}</style>", pagina)
    pagina = re.sub(r'<script type="module" src="([^"]+)"></script>',
                    lambda m: f'<script type="module">\n{definir_modo(juntar_modulos(pasta / m[1]))}</script>', pagina)
    sobrou = re.search(r'(?:src|href)="(?!https?:)([^"]+\.(?:css|js))"', pagina)
    if sobrou:  # o ESP32 não serve esse arquivo
        sys.exit(f"{caminho.name}: {sobrou[1]} não foi embutido. Use <link rel=\"stylesheet\" href=\"...\"> "
                 f"ou <script type=\"module\" src=\"...\"></script>")
    return pagina.encode("utf-8")


linhas = [
    "// Gerado por esp32/gerar_pagina.py a partir de html/. Não edite à mão.",
    "#pragma once",
    "#include <Arduino.h>",
    "",
]
for nome, caminho, comprimir in ARQUIVOS:
    dados = montar_pagina(caminho) if caminho.suffix == ".html" else caminho.read_bytes()
    if comprimir:
        dados = gzip.compress(dados, compresslevel=9, mtime=0)
    linhas.append(f"// {caminho.relative_to(RAIZ)} ({len(dados)} bytes)")
    linhas.append(f"const uint8_t {nome}[] PROGMEM = {{")
    for i in range(0, len(dados), 24):
        linhas.append("  " + ",".join(f"0x{b:02x}" for b in dados[i:i + 24]) + ",")
    linhas.append("};")
    linhas.append(f"const size_t {nome}_LEN = sizeof({nome});")
    linhas.append("")

SAIDA.write_text("\n".join(linhas), encoding="utf-8")
print(f"{SAIDA.relative_to(RAIZ)} gerado (modo {MODO})")
