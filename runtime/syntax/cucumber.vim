" Vim syntax file
" Language:     Cucumber
" Maintainer:   Tim Pope <vimNOSPAM@tpope.org>
" Filenames:    *.feature

if exists("b:current_syntax")
    finish
endif

syn case match
syn sync minlines=20

let g:cucumber_languages = {
      \"en": {"and": "And\\>", "background": "Background\\>", "but": "But\\>", "examples": "Scenarios\\>\\|Examples\\>", "feature": "Feature\\>", "given": "Given\\>", "scenario": "Scenario\\>", "scenario_outline": "Scenario Outline\\>", "then": "Then\\>", "when": "When\\>"},
      \"ar": {"and": "\\%u0648\\>", "background": "\\%u0627\\%u0644\\%u062e\\%u0644\\%u0641\\%u064a\\%u0629\\>", "but": "\\%u0644\\%u0643\\%u0646\\>", "examples": "\\%u0627\\%u0645\\%u062b\\%u0644\\%u0629\\>", "feature": "\\%u062e\\%u0627\\%u0635\\%u064a\\%u0629\\>", "given": "\\%u0628\\%u0641\\%u0631\\%u0636\\>", "scenario": "\\%u0633\\%u064a\\%u0646\\%u0627\\%u0631\\%u064a\\%u0648\\>", "scenario_outline": "\\%u0633\\%u064a\\%u0646\\%u0627\\%u0631\\%u064a\\%u0648 \\%u0645\\%u062e\\%u0637\\%u0637\\>", "then": "\\%u0627\\%u0630\\%u0627\\%u064b\\>\\|\\%u062b\\%u0645\\>", "when": "\\%u0639\\%u0646\\%u062f\\%u0645\\%u0627\\>\\|\\%u0645\\%u062a\\%u0649\\>"},
      \"bg": {"and": "\\%u0418\\>", "background": "\\%u041f\\%u0440\\%u0435\\%u0434\\%u0438\\%u0441\\%u0442\\%u043e\\%u0440\\%u0438\\%u044f\\>", "but": "\\%u041d\\%u043e\\>", "examples": "\\%u041f\\%u0440\\%u0438\\%u043c\\%u0435\\%u0440\\%u0438\\>", "feature": "\\%u0424\\%u0443\\%u043d\\%u043a\\%u0446\\%u0438\\%u043e\\%u043d\\%u0430\\%u043b\\%u043d\\%u043e\\%u0441\\%u0442\\>", "given": "\\%u0414\\%u0430\\%u0434\\%u0435\\%u043d\\%u043e\\>", "scenario": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0439\\>", "scenario_outline": "\\%u0420\\%u0430\\%u043c\\%u043a\\%u0430 \\%u043d\\%u0430 \\%u0441\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0439\\>", "then": "\\%u0422\\%u043e\\>", "when": "\\%u041a\\%u043e\\%u0433\\%u0430\\%u0442\\%u043e\\>"},
      \"cat": {"and": "I\\>", "background": "Antecedents\\>\\|Rerefons\\>", "but": "Per\\%u00f2\\>", "examples": "Exemples\\>", "feature": "Caracter\\%u00edstica\\>", "given": "Donada\\>\\|Donat\\>", "scenario": "Escenari\\>", "scenario_outline": "Esquema de l'escenari\\>", "then": "Aleshores\\>", "when": "Quan\\>"},
      \"cs": {"and": "A tak\\%u00e9\\>\\|A\\>", "background": "Pozad\\%u00ed\\>\\|Kontext\\>", "but": "Ale\\>", "examples": "P\\%u0159\\%u00edklady\\>", "feature": "Po\\%u017eadavek\\>", "given": "Pokud\\>", "scenario": "Sc\\%u00e9n\\%u00e1\\%u0159\\>", "scenario_outline": "N\\%u00e1\\%u010drt Sc\\%u00e9n\\%u00e1\\%u0159e\\>\\|Osnova sc\\%u00e9n\\%u00e1\\%u0159e\\>", "then": "Pak\\>", "when": "Kdy\\%u017e\\>"},
      \"cy": {"and": "A\\>", "background": "Cefndir\\>", "but": "Ond\\>", "examples": "Enghreifftiau\\>", "feature": "Arwedd\\>", "given": "anrhegedig a\\>", "scenario": "Scenario\\>", "scenario_outline": "Scenario Amlinellol\\>", "then": "Yna\\>", "when": "Pryd\\>"},
      \"da": {"and": "Og\\>", "background": "Baggrund\\>", "but": "Men\\>", "examples": "Eksempler\\>", "feature": "Egenskab\\>", "given": "Givet\\>", "scenario": "Scenarie\\>", "scenario_outline": "Abstrakt Scenario\\>", "then": "S\\%u00e5\\>", "when": "N\\%u00e5r\\>"},
      \"de": {"and": "Und\\>", "background": "Grundlage\\>", "but": "Aber\\>", "examples": "Beispiele\\>", "feature": "Funktionalit\\%u00e4t\\>", "given": "Gegeben sei\\>", "scenario": "Szenario\\>", "scenario_outline": "Szenariogrundriss\\>", "then": "Dann\\>", "when": "Wenn\\>"},
      \"en-au": {"and": "N\\>", "background": "Background\\>", "but": "Cept\\>", "examples": "Cobber\\>", "feature": "Crikey\\>", "given": "Ya know how\\>", "scenario": "Mate\\>", "scenario_outline": "Blokes\\>", "then": "Ya gotta\\>", "when": "When\\>"},
      \"en-lol": {"and": "AN\\>", "background": "B4\\>", "but": "BUT\\>", "examples": "EXAMPLZ\\>", "feature": "OH HAI\\>", "given": "I CAN HAZ\\>", "scenario": "MISHUN\\>", "scenario_outline": "MISHUN SRSLY\\>", "then": "DEN\\>", "when": "WEN\\>"},
      \"es": {"and": "Y\\>", "background": "Antecedentes\\>", "but": "Pero\\>", "examples": "Ejemplos\\>", "feature": "Caracter\\%u00edstica\\>", "given": "Dado\\>", "scenario": "Escenario\\>", "scenario_outline": "Esquema del escenario\\>", "then": "Entonces\\>", "when": "Cuando\\>"},
      \"et": {"and": "Ja\\>", "background": "Taust\\>", "but": "Kuid\\>", "examples": "Juhtumid\\>", "feature": "Omadus\\>", "given": "Eeldades\\>", "scenario": "Stsenaarium\\>", "scenario_outline": "Raamstsenaarium\\>", "then": "Siis\\>", "when": "Kui\\>"},
      \"fi": {"and": "Ja\\>", "background": "Tausta\\>", "but": "Mutta\\>", "examples": "Tapaukset\\>", "feature": "Ominaisuus\\>", "given": "Oletetaan\\>", "scenario": "Tapaus\\>", "scenario_outline": "Tapausaihio\\>", "then": "Niin\\>", "when": "Kun\\>"},
      \"fr": {"and": "Et\\>", "background": "Contexte\\>", "but": "Mais\\>", "examples": "Exemples\\>", "feature": "Fonctionnalit\\%u00e9\\>", "given": "Etant donn\\%u00e9\\>\\|Soit\\>", "scenario": "Sc\\%u00e9nario\\>", "scenario_outline": "Plan du sc\\%u00e9nario\\>\\|Plan du Sc\\%u00e9nario\\>", "then": "Alors\\>", "when": "Lorsqu'\\|Lorsque\\>\\|Quand\\>"},
      \"he": {"and": "\\%u05d5\\%u05d2\\%u05dd\\>", "background": "\\%u05e8\\%u05e7\\%u05e2\\>", "but": "\\%u05d0\\%u05d1\\%u05dc\\>", "examples": "\\%u05d3\\%u05d5\\%u05d2\\%u05de\\%u05d0\\%u05d5\\%u05ea\\>", "feature": "\\%u05ea\\%u05db\\%u05d5\\%u05e0\\%u05d4\\>", "given": "\\%u05d1\\%u05d4\\%u05d9\\%u05e0\\%u05ea\\%u05df\\>", "scenario": "\\%u05ea\\%u05e8\\%u05d7\\%u05d9\\%u05e9\\>", "scenario_outline": "\\%u05ea\\%u05d1\\%u05e0\\%u05d9\\%u05ea \\%u05ea\\%u05e8\\%u05d7\\%u05d9\\%u05e9\\>", "then": "\\%u05d0\\%u05d6\\%u05d9\\>\\|\\%u05d0\\%u05d6\\>", "when": "\\%u05db\\%u05d0\\%u05e9\\%u05e8\\>"},
      \"hr": {"and": "I\\>", "background": "Pozadina\\>", "but": "Ali\\>", "examples": "Scenariji\\>\\|Primjeri\\>", "feature": "Mogu\\%u0107nost\\>\\|Mogucnost\\>\\|Osobina\\>", "given": "Zadano\\>\\|Zadani\\>\\|Zadan\\>", "scenario": "Scenarij\\>", "scenario_outline": "Koncept\\>\\|Skica\\>", "then": "Onda\\>", "when": "Kada\\>\\|Kad\\>"},
      \"hu": {"and": "\\%u00c9s\\>", "background": "H\\%u00e1tt\\%u00e9r\\>", "but": "De\\>", "examples": "P\\%u00e9ld\\%u00e1k\\>", "feature": "Jellemz\\%u0151\\>", "given": "Ha\\>", "scenario": "Forgat\\%u00f3k\\%u00f6nyv\\>", "scenario_outline": "Forgat\\%u00f3k\\%u00f6nyv v\\%u00e1zlat\\>", "then": "Akkor\\>", "when": "Majd\\>"},
      \"id": {"and": "Dan\\>", "background": "Dasar\\>", "but": "Tapi\\>", "examples": "Contoh\\>", "feature": "Fitur\\>", "given": "Dengan\\>", "scenario": "Skenario\\>", "scenario_outline": "Skenario konsep\\>", "then": "Maka\\>", "when": "Ketika\\>"},
      \"it": {"and": "E\\>", "background": "Contesto\\>", "but": "Ma\\>", "examples": "Esempi\\>", "feature": "Funzionalit\\%u00e0\\>", "given": "Dato\\>", "scenario": "Scenario\\>", "scenario_outline": "Schema dello scenario\\>", "then": "Allora\\>", "when": "Quando\\>"},
      \"ja": {"and": "\\%u304b\\%u3064", "background": "\\%u80cc\\%u666f\\>", "but": "\\%u3057\\%u304b\\%u3057\\|\\%u305f\\%u3060\\%u3057\\|\\%u4f46\\%u3057", "examples": "\\%u30b5\\%u30f3\\%u30d7\\%u30eb\\>\\|\\%u4f8b\\>", "feature": "\\%u30d5\\%u30a3\\%u30fc\\%u30c1\\%u30e3\\>\\|\\%u6a5f\\%u80fd\\>", "given": "\\%u524d\\%u63d0", "scenario": "\\%u30b7\\%u30ca\\%u30ea\\%u30aa\\>", "scenario_outline": "\\%u30b7\\%u30ca\\%u30ea\\%u30aa\\%u30a2\\%u30a6\\%u30c8\\%u30e9\\%u30a4\\%u30f3\\>\\|\\%u30b7\\%u30ca\\%u30ea\\%u30aa\\%u30c6\\%u30f3\\%u30d7\\%u30ec\\%u30fc\\%u30c8\\>\\|\\%u30b7\\%u30ca\\%u30ea\\%u30aa\\%u30c6\\%u30f3\\%u30d7\\%u30ec\\>\\|\\%u30c6\\%u30f3\\%u30d7\\%u30ec\\>", "then": "\\%u306a\\%u3089\\%u3070", "when": "\\%u3082\\%u3057"},
      \"ko": {"and": "\\%uadf8\\%ub9ac\\%uace0", "background": "\\%ubc30\\%uacbd\\>", "but": "\\%ud558\\%uc9c0\\%ub9cc", "examples": "\\%uc608\\>", "feature": "\\%uae30\\%ub2a5\\>", "given": "\\%uc870\\%uac74", "scenario": "\\%uc2dc\\%ub098\\%ub9ac\\%uc624\\>", "scenario_outline": "\\%uc2dc\\%ub098\\%ub9ac\\%uc624 \\%uac1c\\%uc694\\>", "then": "\\%uadf8\\%ub7ec\\%uba74", "when": "\\%ub9cc\\%uc77c"},
      \"lt": {"and": "Ir\\>", "background": "Kontekstas\\>", "but": "Bet\\>", "examples": "Pavyzd\\%u017eiai\\>\\|Scenarijai\\>\\|Variantai\\>", "feature": "Savyb\\%u0117\\>", "given": "Duota\\>", "scenario": "Scenarijus\\>", "scenario_outline": "Scenarijaus \\%u0161ablonas\\>", "then": "Tada\\>", "when": "Kai\\>"},
      \"lv": {"and": "Un\\>", "background": "Situ\\%u0101cija\\>\\|Konteksts\\>", "but": "Bet\\>", "examples": "Piem\\%u0113ri\\>\\|Paraugs\\>", "feature": "Funkcionalit\\%u0101te\\>\\|F\\%u012b\\%u010da\\>", "given": "Kad\\>", "scenario": "Scen\\%u0101rijs\\>", "scenario_outline": "Scen\\%u0101rijs p\\%u0113c parauga\\>", "then": "Tad\\>", "when": "Ja\\>"},
      \"nl": {"and": "En\\>", "background": "Achtergrond\\>", "but": "Maar\\>", "examples": "Voorbeelden\\>", "feature": "Functionaliteit\\>", "given": "Gegeven\\>\\|Stel\\>", "scenario": "Scenario\\>", "scenario_outline": "Abstract Scenario\\>", "then": "Dan\\>", "when": "Als\\>"},
      \"no": {"and": "Og\\>", "background": "Bakgrunn\\>", "but": "Men\\>", "examples": "Eksempler\\>", "feature": "Egenskap\\>", "given": "Gitt\\>", "scenario": "Scenario\\>", "scenario_outline": "Abstrakt Scenario\\>", "then": "S\\%u00e5\\>", "when": "N\\%u00e5r\\>"},
      \"pl": {"and": "Oraz\\>", "background": "Za\\%u0142o\\%u017cenia\\>", "but": "Ale\\>", "examples": "Przyk\\%u0142ady\\>", "feature": "W\\%u0142a\\%u015bciwo\\%u015b\\%u0107\\>", "given": "Zak\\%u0142adaj\\%u0105c\\>", "scenario": "Scenariusz\\>", "scenario_outline": "Szablon scenariusza\\>", "then": "Wtedy\\>", "when": "Je\\%u017celi\\>"},
      \"pt": {"and": "E\\>", "background": "Contexto\\>", "but": "Mas\\>", "examples": "Exemplos\\>", "feature": "Funcionalidade\\>", "given": "Dado\\>", "scenario": "Cen\\%u00e1rio\\>\\|Cenario\\>", "scenario_outline": "Esquema do Cen\\%u00e1rio\\>\\|Esquema do Cenario\\>", "then": "Ent\\%u00e3o\\>\\|Entao\\>", "when": "Quando\\>"},
      \"ro": {"and": "Si\\>", "background": "Conditii\\>", "but": "Dar\\>", "examples": "Exemplele\\>", "feature": "Functionalitate\\>", "given": "Daca\\>", "scenario": "Scenariu\\>", "scenario_outline": "Scenariul de sablon\\>", "then": "Atunci\\>", "when": "Cand\\>"},
      \"ro2": {"and": "\\%u0218i\\>", "background": "Condi\\%u0163ii\\>", "but": "Dar\\>", "examples": "Exemplele\\>", "feature": "Func\\%u021bionalitate\\>", "given": "Dac\\%u0103\\>", "scenario": "Scenariu\\>", "scenario_outline": "Scenariul de \\%u015fablon\\>", "then": "Atunci\\>", "when": "C\\%u00e2nd\\>"},
      \"ru": {"and": "\\%u041a \\%u0442\\%u043e\\%u043c\\%u0443 \\%u0436\\%u0435\\>\\|\\%u0418\\>", "background": "\\%u041f\\%u0440\\%u0435\\%u0434\\%u044b\\%u0441\\%u0442\\%u043e\\%u0440\\%u0438\\%u044f\\>", "but": "\\%u041d\\%u043e\\>\\|\\%u0410\\>", "examples": "\\%u0417\\%u043d\\%u0430\\%u0447\\%u0435\\%u043d\\%u0438\\%u044f\\>", "feature": "\\%u0424\\%u0443\\%u043d\\%u043a\\%u0446\\%u0438\\%u043e\\%u043d\\%u0430\\%u043b\\>", "given": "\\%u0414\\%u043e\\%u043f\\%u0443\\%u0441\\%u0442\\%u0438\\%u043c\\>", "scenario": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0439\\>", "scenario_outline": "\\%u0421\\%u0442\\%u0440\\%u0443\\%u043a\\%u0442\\%u0443\\%u0440\\%u0430 \\%u0441\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u044f\\>", "then": "\\%u0422\\%u043e\\>", "when": "\\%u0415\\%u0441\\%u043b\\%u0438\\>"},
      \"se": {"and": "Och\\>", "background": "Bakgrund\\>", "but": "Men\\>", "examples": "Exempel\\>", "feature": "Egenskap\\>", "given": "Givet\\>", "scenario": "Scenario\\>", "scenario_outline": "Abstrakt Scenario\\>", "then": "S\\%u00e5\\>", "when": "N\\%u00e4r\\>"},
      \"sk": {"and": "A\\>", "background": "Pozadie\\>", "but": "Ale\\>", "examples": "Pr\\%u00edklady\\>", "feature": "Po\\%u017eiadavka\\>", "given": "Pokia\\%u013e\\>", "scenario": "Scen\\%u00e1r\\>", "scenario_outline": "N\\%u00e1\\%u010drt Scen\\%u00e1ru\\>", "then": "Tak\\>", "when": "Ke\\%u010f\\>"},
      \"sr": {"and": "\\%u0418\\>", "background": "\\%u041a\\%u043e\\%u043d\\%u0442\\%u0435\\%u043a\\%u0441\\%u0442\\>\\|\\%u041f\\%u043e\\%u0437\\%u0430\\%u0434\\%u0438\\%u043d\\%u0430\\>\\|\\%u041e\\%u0441\\%u043d\\%u043e\\%u0432\\%u0430\\>", "but": "\\%u0410\\%u043b\\%u0438\\>", "examples": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0458\\%u0438\\>\\|\\%u041f\\%u0440\\%u0438\\%u043c\\%u0435\\%u0440\\%u0438\\>", "feature": "\\%u0424\\%u0443\\%u043d\\%u043a\\%u0446\\%u0438\\%u043e\\%u043d\\%u0430\\%u043b\\%u043d\\%u043e\\%u0441\\%u0442\\>\\|\\%u041c\\%u043e\\%u0433\\%u0443\\%u045b\\%u043d\\%u043e\\%u0441\\%u0442\\>\\|\\%u041e\\%u0441\\%u043e\\%u0431\\%u0438\\%u043d\\%u0430\\>", "given": "\\%u0417\\%u0430\\%u0434\\%u0430\\%u0442\\%u043e\\>\\|\\%u0417\\%u0430\\%u0434\\%u0430\\%u0442\\%u0435\\>\\|\\%u0417\\%u0430\\%u0434\\%u0430\\%u0442\\%u0438\\>", "scenario": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u043e\\>\\|\\%u041f\\%u0440\\%u0438\\%u043c\\%u0435\\%u0440\\>", "scenario_outline": "\\%u0421\\%u0442\\%u0440\\%u0443\\%u043a\\%u0442\\%u0443\\%u0440\\%u0430 \\%u0441\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0458\\%u0430\\>\\|\\%u041a\\%u043e\\%u043d\\%u0446\\%u0435\\%u043f\\%u0442\\>\\|\\%u0421\\%u043a\\%u0438\\%u0446\\%u0430\\>", "then": "\\%u041e\\%u043d\\%u0434\\%u0430\\>", "when": "\\%u041a\\%u0430\\%u0434\\%u0430\\>\\|\\%u041a\\%u0430\\%u0434\\>"},
      \"sr-Latn": {"and": "I\\>", "background": "Kontekst\\>\\|Pozadina\\>\\|Osnova\\>", "but": "Ali\\>", "examples": "Scenariji\\>\\|Primeri\\>", "feature": "Mogu\\%u0107nost\\>\\|Funkcionalnost\\>\\|Mogucnost\\>\\|Osobina\\>", "given": "Zadato\\>\\|Zadate\\>\\|Zatati\\>", "scenario": "Scenario\\>\\|Primer\\>", "scenario_outline": "Struktura scenarija\\>\\|Koncept\\>\\|Skica\\>", "then": "Onda\\>", "when": "Kada\\>\\|Kad\\>"},
      \"tr": {"and": "Ve\\>", "background": "Ge\\%u00e7mi\\%u015f\\>", "but": "Fakat\\>\\|Ama\\>", "examples": "\\%u00d6rnekler\\>", "feature": "\\%u00d6zellik\\>", "given": "Diyelim ki\\>", "scenario": "Senaryo\\>", "scenario_outline": "Senaryo tasla\\%u011f\\%u0131\\>", "then": "O zaman\\>", "when": "E\\%u011fer ki\\>"},
      \"uz": {"and": "\\%u0412\\%u0430\\>", "background": "\\%u0422\\%u0430\\%u0440\\%u0438\\%u0445\\>", "but": "\\%u041b\\%u0435\\%u043a\\%u0438\\%u043d\\>\\|\\%u0411\\%u0438\\%u0440\\%u043e\\%u043a\\>\\|\\%u0410\\%u043c\\%u043c\\%u043e\\>", "examples": "\\%u041c\\%u0438\\%u0441\\%u043e\\%u043b\\%u043b\\%u0430\\%u0440\\>", "feature": "\\%u0424\\%u0443\\%u043d\\%u043a\\%u0446\\%u0438\\%u043e\\%u043d\\%u0430\\%u043b\\>", "given": "\\%u0410\\%u0433\\%u0430\\%u0440\\>", "scenario": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0439\\>", "scenario_outline": "\\%u0421\\%u0446\\%u0435\\%u043d\\%u0430\\%u0440\\%u0438\\%u0439 \\%u0441\\%u0442\\%u0440\\%u0443\\%u043a\\%u0442\\%u0443\\%u0440\\%u0430\\%u0441\\%u0438\\>", "then": "\\%u0423\\%u043d\\%u0434\\%u0430\\>", "when": "\\%u0410\\%u0433\\%u0430\\%u0440\\>"},
      \"vi": {"and": "V\\%u00e0\\>", "background": "B\\%u1ed1i c\\%u1ea3nh\\>", "but": "Nh\\%u01b0ng\\>", "examples": "D\\%u1eef li\\%u1ec7u\\>", "feature": "T\\%u00ednh n\\%u0103ng\\>", "given": "Bi\\%u1ebft\\>\\|Cho\\>", "scenario": "T\\%u00ecnh hu\\%u1ed1ng\\>\\|K\\%u1ecbch b\\%u1ea3n\\>", "scenario_outline": "Khung t\\%u00ecnh hu\\%u1ed1ng\\>\\|Khung k\\%u1ecbch b\\%u1ea3n\\>", "then": "Th\\%u00ec\\>", "when": "Khi\\>"},
      \"zh-CN": {"and": "\\%u800c\\%u4e14", "background": "\\%u80cc\\%u666f\\>", "but": "\\%u4f46\\%u662f", "examples": "\\%u4f8b\\%u5b50\\>", "feature": "\\%u529f\\%u80fd\\>", "given": "\\%u5047\\%u5982", "scenario": "\\%u573a\\%u666f\\>", "scenario_outline": "\\%u573a\\%u666f\\%u5927\\%u7eb2\\>", "then": "\\%u90a3\\%u4e48", "when": "\\%u5f53"},
      \"zh-TW": {"and": "\\%u800c\\%u4e14\\|\\%u4e26\\%u4e14", "background": "\\%u80cc\\%u666f\\>", "but": "\\%u4f46\\%u662f", "examples": "\\%u4f8b\\%u5b50\\>", "feature": "\\%u529f\\%u80fd\\>", "given": "\\%u5047\\%u8a2d", "scenario": "\\%u5834\\%u666f\\>\\|\\%u5287\\%u672c\\>", "scenario_outline": "\\%u5834\\%u666f\\%u5927\\%u7db1\\>\\|\\%u5287\\%u672c\\%u5927\\%u7db1\\>", "then": "\\%u90a3\\%u9ebc", "when": "\\%u7576"}}

function! s:pattern(key)
  let language = matchstr(getline(1),'#\s*language:\s*\zs\S\+')
  if has_key(g:cucumber_languages, language)
    let languages = [g:cucumber_languages[language]]
  else
    let languages = values(g:cucumber_languages)
  end
  return '\<\%('.join(map(languages,'get(v:val,a:key,"\\%(a\\&b\\)")'),'\|').'\)'
endfunction

function! s:Add(name)
  let next = " skipempty skipwhite nextgroup=".join(map(["Region","AndRegion","ButRegion","Comment","Table"],'"cucumber".a:name.v:val'),",")
  exe "syn region cucumber".a:name.'Region matchgroup=cucumber'.a:name.' start="\%(^\s*\)\@<=\%('.s:pattern(tolower(a:name)).'\)" end="$"'.next
  exe 'syn region cucumber'.a:name.'AndRegion matchgroup=cucumber'.a:name.'And start="\%(^\s*\)\@<='.s:pattern('and').'" end="$" contained'.next
  exe 'syn region cucumber'.a:name.'ButRegion matchgroup=cucumber'.a:name.'But start="\%(^\s*\)\@<='.s:pattern('but').'" end="$" contained'.next
  exe 'syn match cucumber'.a:name.'Comment "\%(^\s*\)\@<=#.*" contained'.next
  exe 'syn match cucumber'.a:name.'Table "\%(^\s*\)\@<=|.*" contained contains=cucumberDelimiter'.next
  exe 'hi def link cucumber'.a:name.'Comment cucumberComment'
  exe 'hi def link cucumber'.a:name.'But cucumber'.a:name.'And'
  exe 'hi def link cucumber'.a:name.'And cucumber'.a:name
  exe 'syn cluster cucumberStepRegions add=cucumber'.a:name.'Region,cucumber'.a:name.'AndRegion,cucumber'.a:name.'ButRegion'
endfunction

syn match   cucumberComment  "\%(^\s*\)\@<=#.*"
syn match   cucumberComment  "\%(\%^\s*\)\@<=#.*" contains=cucumberLanguage
syn match   cucumberLanguage "\%(#\s*\)\@<=language:" contained
syn match   cucumberUnparsed "\S.*" nextgroup=cucumberUnparsedComment,cucumberUnparsed,cucumberTags,cucumberBackground,cucumberScenario,cucumberScenarioOutline,cucumberExamples skipwhite skipempty contained
syn match   cucumberUnparsedComment "#.*" nextgroup=cucumberUnparsedComment,cucumberUnparsed,cucumberTags,cucumberBackground,cucumberScenario,cucumberScenarioOutline,cucumberExamples skipwhite skipempty contained

exe 'syn match cucumberFeature "\%(^\s*\)\@<='.s:pattern('feature').':" nextgroup=cucumberUnparsedComment,cucumberUnparsed,cucumberBackground,cucumberScenario,cucumberScenarioOutline,cucumberExamples skipwhite skipempty'
exe 'syn match cucumberBackground "\%(^\s*\)\@<='.s:pattern('background').':"'
exe 'syn match cucumberScenario "\%(^\s*\)\@<='.s:pattern('scenario').':"'
exe 'syn match cucumberScenarioOutline "\%(^\s*\)\@<='.s:pattern('scenario_outline').':"'
exe 'syn match cucumberExamples "\%(^\s*\)\@<='.s:pattern('examples').':" nextgroup=cucumberExampleTable skipempty skipwhite'

syn match   cucumberPlaceholder   "<[^<>]*>" contained containedin=@cucumberStepRegions
syn match   cucumberExampleTable  "\%(^\s*\)\@<=|.*" contains=cucumberDelimiter
syn match   cucumberDelimiter     "|" contained
syn match   cucumberTags          "\%(^\s*\)\@<=\%(@[^@[:space:]]\+\s\+\)*@[^@[:space:]]\+\s*$"
syn region  cucumberString   start=+\%(^\s*\)\@<="""+ end=+"""+

call s:Add('Then')
call s:Add('When')
call s:Add('Given')

hi def link cucumberUnparsedComment   cucumberComment
hi def link cucumberComment           Comment
hi def link cucumberLanguage          SpecialComment
hi def link cucumberFeature           Macro
hi def link cucumberBackground        Define
hi def link cucumberScenario          Define
hi def link cucumberScenarioOutline   Define
hi def link cucumberExamples          Define
hi def link cucumberPlaceholder       Constant
hi def link cucumberDelimiter         Delimiter
hi def link cucumberTags              Tag
hi def link cucumberString            String
hi def link cucumberGiven             Conditional
hi def link cucumberWhen              Function
hi def link cucumberThen              Type

let b:current_syntax = "cucumber"

" vim:set sts=2 sw=2:
