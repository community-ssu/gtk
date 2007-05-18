#define N_(s) s

/* The paper size names are from:
 * PWG 5101.1-2002 PWG: Standard for Media Standardized Names
 *
 * The PPD names come from the PPD specification.
 */

typedef struct {
  const gchar *name;
  const gchar *size;
  const gchar *display_name;
  const gchar *ppd_name;
} PaperInfo;

static const PaperInfo standard_names[] = {
  /* sorted by name, remember to sort when changing */
  {"asme_f", "28x40in", N_("asme_f")}, /* f           5    e1 */
  {"iso_2a0", "1189x1682mm", N_("A0x2")},
  {"iso_a0", "841x1189mm", N_("A0"), "A0"},
  {"iso_a0x3", "1189x2523mm", N_("A0x3")},
  {"iso_a1", "594x841mm", N_("A1"), "A1"},
  {"iso_a10", "26x37mm", N_("A10"), "A10"},
  {"iso_a1x3", "841x1783mm", N_("A1x3")},
  {"iso_a1x4", "841x2378mm", N_("A1x4")}, 
  {"iso_a2", "420x594mm", N_("A2"), "A2"},
  {"iso_a2x3", "594x1261mm", N_("A2x3")},
  {"iso_a2x4", "594x1682mm", N_("A2x4")},
  {"iso_a2x5", "594x2102mm", N_("A2x5")},
  {"iso_a3", "297x420mm", N_("A3"), "A3"},
  {"iso_a3-extra", "322x445mm", N_("A3 Extra"), "A3Extra"},
  {"iso_a3x3", "420x891mm", N_("A3x3")},
  {"iso_a3x4", "420x1189mm", N_("A3x4")},
  {"iso_a3x5", "420x1486mm", N_("A3x5")},
  {"iso_a3x6", "420x1783mm", N_("A3x6")},
  {"iso_a3x7", "420x2080mm", N_("A3x7")},
  {"iso_a4", "210x297mm", N_("A4"), "A4"},
  {"iso_a4-extra", "235.5x322.3mm", N_("A4 Extra"), "A4Extra"},
  {"iso_a4-tab", "225x297mm", N_("A4 Tab")},
  {"iso_a4x3", "297x630mm", N_("A4x3")},
  {"iso_a4x4", "297x841mm", N_("A4x4")},
  {"iso_a4x5", "297x1051mm", N_("A4x5")},
  {"iso_a4x6", "297x1261mm", N_("A4x6")},
  {"iso_a4x7", "297x1471mm", N_("A4x7")},
  {"iso_a4x8", "297x1682mm", N_("A4x8")},
  {"iso_a4x9", "297x1892mm", N_("A4x9")},
  {"iso_a5", "148x210mm", N_("A5"), "A5"},
  {"iso_a5-extra", "174x235mm", N_("A5 Extra"), "A5Extra"},
  {"iso_a6", "105x148mm", N_("A6"), "A6"},
  {"iso_a7", "74x105mm", N_("A7"), "A7"},
  {"iso_a8", "52x74mm", N_("A8"), "A8"},
  {"iso_a9", "37x52mm", N_("A9"), "A9"},
  {"iso_b0", "1000x1414mm", N_("B0"), "ISOB0"},
  {"iso_b1", "707x1000mm", N_("B1"), "ISOB1"},
  {"iso_b10", "31x44mm", N_("B10"), "ISOB10"},
  {"iso_b2", "500x707mm", N_("B2"), "ISOB2"},
  {"iso_b3", "353x500mm", N_("B3"), "ISOB3"},
  {"iso_b4", "250x353mm", N_("B4"), "ISOB4"},
  {"iso_b5", "176x250mm", N_("B5"), "ISOB5"},
  {"iso_b5-extra", "201x276mm", N_("B5 Extra"), "ISOB5Extra"},
  {"iso_b6", "125x176mm", N_("B6"), "ISOB6"}, 
  {"iso_b6c4", "125x324mm", N_("B6/C4")}, /* b6/c4 Envelope */
  {"iso_b7", "88x125mm", N_("B7"), "ISOB7"},
  {"iso_b8", "62x88mm", N_("B8"), "ISOB8"},
  {"iso_b9", "44x62mm", N_("B9"), "ISOB9"},
  {"iso_c0", "917x1297mm", N_("C0"), "EnvC0"},
  {"iso_c1", "648x917mm", N_("C1"), "EnvC1"},
  {"iso_c10", "28x40mm", N_("C10"), "EnvC10"},
  {"iso_c2", "458x648mm", N_("C2"), "EnvC2"},
  {"iso_c3", "324x458mm", N_("C3"), "EnvC3"},
  {"iso_c4", "229x324mm", N_("C4"), "EnvC4"},
  {"iso_c5", "162x229mm", N_("C5"), "EnvC5"},
  {"iso_c6", "114x162mm", N_("C6"), "EnvC6"},
  {"iso_c6c5", "114x229mm", N_("C6/C5"), "EnvC65"},
  {"iso_c7", "81x114mm", N_("C7"), "EnvC7"},
  {"iso_c7c6", "81x162mm", N_("C7/C6")}, /* c7/c6 Envelope */
  {"iso_c8", "57x81mm", N_("C8"), "EnvC8"},
  {"iso_c9", "40x57mm", N_("C9"), "EnvC9"},
  {"iso_dl", "110x220mm", N_("DL Envelope"), "EnvDL"}, /* iso-designated 1, 2 designated-long, dl Envelope */
  {"iso_ra0", "860x1220mm", N_("RA0")},
  {"iso_ra1", "610x860mm", N_("RA1")},
  {"iso_ra2", "430x610mm", N_("RA2")},
  {"iso_sra0", "900x1280mm", N_("SRA0")},
  {"iso_sra1", "640x900mm", N_("SRA1")},
  {"iso_sra2", "450x640mm", N_("SRA2")},
  {"jis_b0", "1030x1456mm", N_("JB0"), "B0"},
  {"jis_b1", "728x1030mm", N_("JB1"), "B1"},
  {"jis_b10", "32x45mm", N_("JB10"), "B10"},
  {"jis_b2", "515x728mm", N_("JB2"), "B2"},
  {"jis_b3", "364x515mm", N_("JB3"), "B3"},
  {"jis_b4", "257x364mm", N_("JB4"), "B4"},
  {"jis_b5", "182x257mm", N_("JB5"), "B5"},
  {"jis_b6", "128x182mm", N_("JB6"), "B6"},
  {"jis_b7", "91x128mm", N_("JB7"), "B7"},
  {"jis_b8", "64x91mm", N_("JB8"), "B8"},
  {"jis_b9", "45x64mm", N_("JB9"), "B9"},
  {"jis_exec", "216x330mm", N_("jis exec")},
  {"jpn_chou2", "111.1x146mm", N_("Choukei 2 Envelope")},
  {"jpn_chou3", "120x235mm", N_("Choukei 3 Envelope"), "EnvChou3"},
  {"jpn_chou4", "90x205mm", N_("Choukei 4 Envelope"), "EnvChou4"},
  {"jpn_hagaki", "100x148mm", N_("hagaki (postcard)"), "Postcard"},
  {"jpn_kahu", "240x322.1mm", N_("kahu Envelope")},
  {"jpn_kaku2", "240x332mm", N_("kaku2 Envelope"), "EnvKaku2"},
  {"jpn_oufuku", "148x200mm", N_("oufuku (reply postcard)"), "DoublePostcard"},
  {"jpn_you4", "105x235mm", N_("you4 Envelope")},
  {"na_10x11", "10x11in", N_("10x11"), "10x11"},
  {"na_10x13", "10x13in", N_("10x13"), "10x13"},
  {"na_10x14", "10x14in", N_("10x14"), "10x14"},
  {"na_10x15", "10x15in", N_("10x15")},
  {"na_10x15", "10x15in", N_("10x15")},
  {"na_11x12", "11x12in", N_("11x12"), "12x11"}, 
  {"na_11x15", "11x15in", N_("11x15"), "15x11"}, 
  {"na_12x19", "12x19in", N_("12x19")},
  {"na_5x7", "5x7in", N_("5x7")},
  {"na_6x9", "6x9in", N_("6x9 Envelope")},
  {"na_7x9", "7x9in", N_("7x9 Envelope"), "7x9"},
  {"na_9x11", "9x11in", N_("9x11 Envelope"), "9x11"},
  {"na_a2", "4.375x5.75in", N_("a2 Envelope")},
  {"na_arch-a", "9x12in", N_("Arch A"), "ARCHA"},
  {"na_arch-b", "12x18in", N_("Arch B"), "ARCHB"},
  {"na_arch-c", "18x24in", N_("Arch C"), "ARCHC"},
  {"na_arch-d", "24x36in", N_("Arch D"), "ARCHD"},
  {"na_arch-e", "36x48in", N_("Arch E"), "ARCHE"},
  {"na_b-plus", "12x19.17in", N_("b-plus")},
  {"na_c", "17x22in", N_("c"), "AnsiC"},
  {"na_c5", "6.5x9.5in", N_("c5 Envelope")},
  {"na_d", "22x34in", N_("d"), "AnsiD"},
  {"na_e", "34x44in", N_("e"), "AnsiE"},
  {"na_edp", "11x14in", N_("edp")},
  {"na_eur-edp", "12x14in", N_("European edp")},
  {"na_executive", "7.25x10.5in", N_("Executive"), "Executive"},
  {"na_f", "44x68in", N_("f")},
  {"na_fanfold-eur", "8.5x12in", N_("FanFold European"), "FanFoldGerman"},
  {"na_fanfold-us", "11x14.875in", N_("FanFold US"), "FanFoldUS"},
  {"na_foolscap", "8.5x13in", N_("FanFold German Legal"), "FanFoldGermanLegal"}, /* foolscap, german-legal-fanfold */
  {"na_govt-legal", "8x13in", N_("Government Legal")},
  {"na_govt-letter", "8x10in", N_("Government Letter"), "8x10"},
  {"na_index-3x5", "3x5in", N_("Index 3x5")},
  {"na_index-4x6", "4x6in", N_("Index 4x6 (postcard)")},
  {"na_index-4x6-ext", "6x8in", N_("Index 4x6 ext")},
  {"na_index-5x8", "5x8in", N_("Index 5x8")},
  {"na_invoice", "5.5x8.5in", N_("Invoice"), "Statement"}, /* invoice,  statement, mini, half-letter */
  {"na_ledger", "11x17in", N_("Tabloid"), "Ledger"}, /* tabloid, engineering-b */
  {"na_legal", "8.5x14in", N_("US Legal"), "Legal"},
  {"na_legal-extra", "9.5x15in", N_("US Legal Extra"), "LegalExtra"},
  {"na_letter", "8.5x11in", N_("US Letter"), "Letter"},
  {"na_letter-extra", "9.5x12in", N_("US Letter Extra"), "LetterExtra"},
  {"na_letter-plus", "8.5x12.69in", N_("US Letter Plus"), "LetterPlus"},
  {"na_monarch", "3.875x7.5in", N_("Monarch Envelope"), "EnvMonarch"},
  {"na_number-10", "4.125x9.5in", N_("#10 Envelope"), "Env10"}, /* na-number-10-envelope 1, 2 comm-10 Envelope */
  {"na_number-11", "4.5x10.375in", N_("#11 Envelope"), "Env11"}, /* number-11 Envelope */
  {"na_number-12", "4.75x11in", N_("#12 Envelope"), "Env12"}, /* number-12 Envelope */
  {"na_number-14", "5x11.5in", N_("#14 Envelope"), "Env14"}, /* number-14 Envelope */
  {"na_number-9", "3.875x8.875in", N_("#9 Envelope"), "Env9"},
  {"na_personal", "3.625x6.5in", N_("Personal Envelope"), "EnvPersonal"},
  {"na_quarto", "8.5x10.83in", N_("Quarto"), "Quarto"}, 
  {"na_super-a", "8.94x14in", N_("Super A"), "SuperA"}, 
  {"na_super-b", "13x19in", N_("Super B"), "SuperB"}, 
  {"na_wide-format", "30x42in", N_("Wide Format")},
  {"om_dai-pa-kai", "275x395mm", N_("Dai-pa-kai")},
  {"om_folio", "210x330mm", N_("Folio"), "Folio"}, 
  {"om_folio-sp", "215x315mm", N_("Folio sp")},
  {"om_invite", "220x220mm", N_("Invite Envelope"), "EnvInvite"},
  {"om_italian", "110x230mm", N_("Italian Envelope"), "EnvItalian"},
  {"om_juuro-ku-kai", "198x275mm", N_("juuro-ku-kai")},
  {"om_pa-kai", "267x389mm", N_("pa-kai")},
  {"om_postfix", "114x229mm", N_("Postfix Envelope")},
  {"om_small-photo", "100x150mm", N_("Small Photo")},
  {"prc_1", "102x165mm", N_("prc1 Envelope"), "EnvPRC1"},
  {"prc_10", "324x458mm", N_("prc10 Envelope"), "EnvPRC10"},
  {"prc_16k", "146x215mm", N_("prc 16k"), "PRC16K"},
  {"prc_2", "102x176mm", N_("prc2 Envelope"), "EnvPRC2"},
  {"prc_3", "125x176mm", N_("prc3 Envelope"), "EnvPRC3"},
  {"prc_32k", "97x151mm", N_("prc 32k"), "PRC32K"},
  {"prc_4", "110x208mm", N_("prc4 Envelope"), "EnvPRC4"},
  {"prc_5", "110x220mm", N_("prc5 Envelope"), "EnvPRC5"},
  {"prc_6", "120x320mm", N_("prc6 Envelope"), "EnvPRC6"},
  {"prc_7", "160x230mm", N_("prc7 Envelope"), "EnvPRC7"},
  {"prc_8", "120x309mm", N_("prc8 Envelope"), "EnvPRC8"},
  {"roc_16k", "7.75x10.75in", N_("ROC 16k")},
  {"roc_8k", "10.75x15.5in", N_("ROC 8k")},
};

/* Some page sizes have multiple PPD names in use.
 * The array above only contails the prefered one,
 * and this array fills out with the duplicates.
 */
static const struct {
  const char *ppd_name;
  const char *standard_name;
} extra_ppd_names[] = {
  /* sorted by ppd_name, remember to sort when changing */
  { "C4", "iso_c4"},
  { "C5", "iso_c5"},
  { "C6", "iso_c6"},
  { "Comm10", "na_number-10"},
  { "DL", "iso_dl"},
  { "Monarch", "na_monarch"},
};
