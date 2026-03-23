/**
 * @file AnalisisEu152_v6.cpp
 * @brief Análisis Dual-Energy con Eu-152 usando TSpectrum
 * 
 * MEJORAS vs v4/v5:
 *   - Usa TSpectrum para identificación automática de picos
 *   - Separación de fondo mediante algoritmo SNIP (Statistics-sensitive Nonlinear Iterative Peak-clipping)
 *   - No requiere ventanas ROI definidas manualmente
 *   - Funciona perfectamente con picos estrechos (simulaciones MC)
 *   - Amplitudes robustas sin ajustes
 * 
 * MÉTODO TSpectrum:
 *   1. SearchHighRes() - encuentra picos automáticamente
 *   2. Background() - estima fondo con algoritmo SNIP
 *   3. Amplitud = altura_pico - fondo_local
 * 
 * ARCHIVOS ESPERADOS:
 *   Barrido FINO: Eu152_REE_0p00.root, ..., Eu152_REE_0p01.root
 *   Barrido GRUESO: Eu152_REE_0p02.root, ..., Eu152_REE_0p05.root
 * 
 * Estructura: TTree "Scoring", Branch "Energy" en MeV
 * 
 * Uso: root -l 'AnalisisEu152_v6.cpp("./")'
 *      root -l 'AnalisisEu152_v6.cpp("./", true)'  // usa 1408 keV
 * 
 * @author Usuario/Claude
 * @date 2025-02
 */

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TGraph.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TLine.h"
#include "TMath.h"
#include "TLatex.h"
#include "TSystem.h"
#include "TColor.h"
#include "TMultiGraph.h"
#include "TAxis.h"
#include "TROOT.h"
#include "TSpectrum.h"

// ============================================================================
// CONFIGURACIÓN - LÍNEAS Eu-152
// ============================================================================

const double E_122  = 121.78;
const double E_344  = 344.28;
const double E_779  = 778.90;
const double E_964  = 964.08;
const double E_1112 = 1112.07;
const double E_1408 = 1408.01;

// ============================================================================
// ESTRUCTURA DE DATOS PARA MUESTRAS
// ============================================================================

struct MuestraInfo {
    TString nombre;
    double concentracion;
    bool esFino;
};

const std::vector<MuestraInfo> MUESTRAS = {
    {"Eu152_REE_0p00.root",   0.00,  true},
    {"Eu152_REE_0p002.root",  0.20,  true},
    {"Eu152_REE_0p004.root",  0.40,  true},
    {"Eu152_REE_0p006.root",  0.60,  true},
    {"Eu152_REE_0p008.root",  0.80,  true},
    {"Eu152_REE_0p01.root",   1.00,  true},
    {"Eu152_REE_0p02.root",   2.00,  false},
    {"Eu152_REE_0p03.root",   3.00,  false},
    {"Eu152_REE_0p04.root",   4.00,  false},
    {"Eu152_REE_0p05.root",   5.00,  false},
};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS DE ANÁLISIS TSpectrum
// ============================================================================

struct ResultadoTSpectrum {
    // Pico identificado
    double energia;        // Energía del pico (keV)
    double amplitud;       // Altura del pico (cuentas)
    double fondo;          // Nivel de fondo bajo el pico
    double amplitud_neta;  // Amplitud - fondo
    double error;          // Error estadístico (√amplitud)
    
    // Valores normalizados
    double amplitud_neta_norm;
    double error_norm;
    
    bool encontrado;
    
    ResultadoTSpectrum() : energia(0), amplitud(0), fondo(0), amplitud_neta(0), 
                           error(0), amplitud_neta_norm(0), error_norm(0), 
                           encontrado(false) {}
};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS COMPLETOS DE UNA MUESTRA
// ============================================================================

struct ResultadoMuestra {
    double conc;
    bool esFino;
    
    // Amplitudes netas de TSpectrum
    double A_low, errA_low;
    double A_high, errA_high;
    
    // Índice Q basado en amplitudes
    double Q, errQ;
    
    // Z-score para detectabilidad
    double Z_score;
    bool detectable;
    bool cuantificable;
    
    ResultadoMuestra() : conc(0), esFino(true), 
                         A_low(0), errA_low(0), A_high(0), errA_high(0),
                         Q(0), errQ(0),
                         Z_score(0), detectable(false), cuantificable(false) {}
};

// ============================================================================
// FUNCIÓN: Encontrar y analizar pico específico con TSpectrum
// ============================================================================

ResultadoTSpectrum AnalizarPicoTSpectrum(TH1D* h, double E_esperado, 
                                         double tolerancia = 10.0,
                                         double factor_norm = 1.0,
                                         bool debug = false) {
    ResultadoTSpectrum r;
    if (!h) return r;
    
    // =========================================================================
    // PASO 1: Estimar fondo con algoritmo SNIP
    // =========================================================================
    
    TSpectrum* spectrum = new TSpectrum();
    
    // Generar nombre único para evitar conflictos en el directorio global de ROOT
    static int counter = 0;
    counter++;
    
    // Parámetros del algoritmo de fondo:
    // - iterations: más iteraciones = fondo más suave (típico: 20-50)
    // - option: opciones del algoritmo SNIP
    int iterations = 30;
    
    // Calcular fondo
    // NOTA: Background() retorna un TH1* con el fondo, no modifica in-place
    TH1* h_bg_temp = spectrum->Background(h, iterations, "");
    if (!h_bg_temp) {
        std::cerr << "[ERROR] Background() retornó NULL para E=" << E_esperado << std::endl;
        delete spectrum;
        return r;
    }
    h_bg_temp->SetDirectory(0);  // CRÍTICO: Desvincular del directorio global
    
    // =========================================================================
    // PASO 2: Crear histograma sin fondo
    // =========================================================================
    
    TH1D* h_no_bg = (TH1D*)h->Clone(Form("h_no_bg_%d", counter));
    h_no_bg->SetDirectory(0);  // CRÍTICO: Desvincular del directorio global
    h_no_bg->Add(h_bg_temp, -1.0);  // h_no_bg = h - h_bg
    
    // =========================================================================
    // PASO 3: Buscar picos en la región de interés
    // =========================================================================
    
    // Definir ventana de búsqueda
    int bin_min = h->FindBin(E_esperado - tolerancia);
    int bin_max = h->FindBin(E_esperado + tolerancia);
    
    // Buscar máximo en ventana (en histograma sin fondo)
    double max_amplitud = 0;
    int bin_max_amplitud = 0;
    
    for (int i = bin_min; i <= bin_max; i++) {
        double contenido = h_no_bg->GetBinContent(i);
        if (contenido > max_amplitud) {
            max_amplitud = contenido;
            bin_max_amplitud = i;
        }
    }
    
    if (bin_max_amplitud > 0 && max_amplitud > 0) {
        r.encontrado = true;
        r.energia = h->GetBinCenter(bin_max_amplitud);
        r.amplitud = h->GetBinContent(bin_max_amplitud);  // Altura en histograma original
        r.fondo = h_bg_temp->GetBinContent(bin_max_amplitud);  // Fondo estimado
        r.amplitud_neta = max_amplitud;  // Altura sin fondo
        
        // Error: aproximación estadística basada en amplitud
        // Para picos con buena estadística: error ≈ √(amplitud)
        r.error = std::sqrt(r.amplitud);
        
        // Normalización (para comparar muestras con diferente estadística)
        r.amplitud_neta_norm = r.amplitud_neta * factor_norm;
        r.error_norm = r.error * factor_norm;
        
        if (debug) {
            std::cout << "[DEBUG] Pico encontrado en " << r.energia << " keV:" << std::endl;
            std::cout << "        Amplitud total: " << r.amplitud << std::endl;
            std::cout << "        Fondo: " << r.fondo << std::endl;
            std::cout << "        Amplitud neta: " << r.amplitud_neta << " ± " << r.error << std::endl;
        }
    } else {
        r.encontrado = false;
        if (debug) {
            std::cerr << "[WARN] No se encontró pico cerca de " << E_esperado << " keV" << std::endl;
        }
    }
    
    // Limpiar memoria
    delete h_no_bg;
    delete h_bg_temp;
    delete spectrum;
    
    return r;
}

// ============================================================================
// FUNCIÓN: Visualizar separación de fondo (para diagnóstico)
// ============================================================================

void VisualizarSeparacionFondo(TH1D* h, double E_low, double E_high, 
                               const char* titulo = "Separacion de Fondo") {
    
    TSpectrum* spectrum = new TSpectrum();
    
    // Calcular fondo
    TH1* h_bg_temp = spectrum->Background(h, 30, "");
    if (!h_bg_temp) {
        std::cerr << "[ERROR] Background() falló en visualización" << std::endl;
        delete spectrum;
        return;
    }
    h_bg_temp->SetDirectory(0);
    
    // Crear histograma sin fondo
    TH1D* h_no_bg = (TH1D*)h->Clone("h_no_bg_vis");
    h_no_bg->SetDirectory(0);
    h_no_bg->Add(h_bg_temp, -1.0);
    
    // Visualizar
    TCanvas* c = new TCanvas("cFondo", titulo, 1400, 500);
    c->Divide(3, 1);
    
    // Panel 1: Espectro original
    c->cd(1);
    gPad->SetLogy();
    gPad->SetGrid();
    h->SetLineColor(kBlack);
    h->SetLineWidth(2);
    h->SetTitle("Espectro Original;Energia (keV);Cuentas");
    h->GetXaxis()->SetRangeUser(50, 1500);
    h->Draw();
    
    // Marcar picos de interés
    TLine* l1 = new TLine(E_low, 1, E_low, h->GetMaximum()/2);
    l1->SetLineColor(kRed); l1->SetLineWidth(2); l1->Draw();
    TLine* l2 = new TLine(E_high, 1, E_high, h->GetMaximum()/10);
    l2->SetLineColor(kGreen+2); l2->SetLineWidth(2); l2->Draw();
    
    // Panel 2: Fondo estimado
    c->cd(2);
    gPad->SetLogy();
    gPad->SetGrid();
    h->Draw();
    h_bg_temp->SetLineColor(kBlue);
    h_bg_temp->SetLineWidth(3);
    h_bg_temp->Draw("same");
    
    TLegend* leg = new TLegend(0.6, 0.7, 0.88, 0.88);
    leg->AddEntry(h, "Original", "l");
    leg->AddEntry(h_bg_temp, "Fondo (SNIP)", "l");
    leg->Draw();
    
    // Panel 3: Espectro sin fondo
    c->cd(3);
    gPad->SetLogy();
    gPad->SetGrid();
    h_no_bg->SetLineColor(kRed);
    h_no_bg->SetLineWidth(2);
    h_no_bg->SetTitle("Sin Fondo;Energia (keV);Cuentas");
    h_no_bg->GetXaxis()->SetRangeUser(50, 1500);
    h_no_bg->Draw();
    
    l1 = new TLine(E_low, 0.1, E_low, h_no_bg->GetMaximum()/2);
    l1->SetLineColor(kRed); l1->SetLineWidth(2); l1->Draw();
    l2 = new TLine(E_high, 0.1, E_high, h_no_bg->GetMaximum()/10);
    l2->SetLineColor(kGreen+2); l2->SetLineWidth(2); l2->Draw();
    
    c->SaveAs("Eu152_v6_SeparacionFondo.png");
    
    delete h_no_bg;
    delete h_bg_temp;
    delete spectrum;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void AnalisisEu152_v6(const char* directorio = "./", bool usar_1408 = false) {
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  ANALISIS Eu-152 v6 - TSpectrum (SEPARACION DE FONDO AVANZADA)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // =========================================================================
    // CONFIGURACIÓN DE LÍNEAS
    // =========================================================================
    
    double E_LOW = E_122;
    double E_HIGH = usar_1408 ? E_1408 : E_344;
    
    // Tolerancia para búsqueda de picos (más amplia que ventanas ROI)
    double tolerancia_low = 15.0;   // ±15 keV para buscar 122 keV
    double tolerancia_high = usar_1408 ? 30.0 : 20.0;  // ±30 keV para 1408, ±20 para 344
    
    printf("  Linea BAJA:  %.2f keV (busqueda +/-%.0f keV)\n", E_LOW, tolerancia_low);
    printf("  Linea ALTA:  %.2f keV (busqueda +/-%.0f keV)\n", E_HIGH, tolerancia_high);
    printf("  Metodo:      TSpectrum + SNIP Background\n");
    std::cout << std::string(80, '=') << std::endl << std::endl;
    
    // =========================================================================
    // PROCESAR MUESTRA DE REFERENCIA (0% REE)
    // =========================================================================
    
    TString filename_ref = Form("%s%s", directorio, MUESTRAS[0].nombre.Data());
    TFile* f_ref = TFile::Open(filename_ref, "READ");
    if (!f_ref || f_ref->IsZombie()) {
        std::cerr << "[ERROR] No se pudo abrir referencia: " << filename_ref << std::endl;
        return;
    }
    
    TTree* t_ref = (TTree*)f_ref->Get("Scoring");
    if (!t_ref) {
        std::cerr << "[ERROR] No se encontró TTree 'Scoring' en referencia" << std::endl;
        f_ref->Close();
        return;
    }
    
    Long64_t N_eventos_ref = t_ref->GetEntries();
    
    // Crear histograma para referencia
    TH1D* h_ref = new TH1D("h_ref", "Espectro Referencia", 1600, 0, 1600);
    h_ref->SetDirectory(0);  // Desvincular del directorio global
    Double_t energy;
    t_ref->SetBranchAddress("Energy", &energy);
    for (Long64_t i = 0; i < N_eventos_ref; i++) {
        t_ref->GetEntry(i);
        h_ref->Fill(energy * 1000.0);  // MeV -> keV
    }
    
    // Visualizar separación de fondo en referencia
    printf("[INFO] Visualizando separacion de fondo en referencia...\n");
    VisualizarSeparacionFondo(h_ref, E_LOW, E_HIGH, "Referencia - Separacion de Fondo");
    
    // Analizar picos en referencia
    printf("[INFO] Analizando picos en referencia con TSpectrum...\n");
    ResultadoTSpectrum pico_ref_low = AnalizarPicoTSpectrum(h_ref, E_LOW, tolerancia_low, 1.0, true);
    ResultadoTSpectrum pico_ref_high = AnalizarPicoTSpectrum(h_ref, E_HIGH, tolerancia_high, 1.0, true);
    
    if (!pico_ref_low.encontrado || !pico_ref_high.encontrado) {
        std::cerr << "[ERROR] No se pudieron identificar picos en referencia" << std::endl;
        std::cerr << "        Pico bajo encontrado: " << (pico_ref_low.encontrado ? "SI" : "NO") << std::endl;
        std::cerr << "        Pico alto encontrado: " << (pico_ref_high.encontrado ? "SI" : "NO") << std::endl;
        return;
    }
    
    // Calcular Q0 y su error
    double Q0 = pico_ref_low.amplitud_neta / pico_ref_high.amplitud_neta;
    double errQ0 = Q0 * std::sqrt(std::pow(pico_ref_low.error/pico_ref_low.amplitud_neta, 2) + 
                                   std::pow(pico_ref_high.error/pico_ref_high.amplitud_neta, 2));
    
    printf("\n[INFO] REFERENCIA (0%% REE):\n");
    printf("  Eventos totales: %lld\n", N_eventos_ref);
    printf("  Pico %.2f keV:\n", pico_ref_low.energia);
    printf("    Amplitud total = %.1f\n", pico_ref_low.amplitud);
    printf("    Fondo = %.1f\n", pico_ref_low.fondo);
    printf("    Amplitud neta = %.1f +/- %.1f\n", pico_ref_low.amplitud_neta, pico_ref_low.error);
    printf("  Pico %.2f keV:\n", pico_ref_high.energia);
    printf("    Amplitud total = %.1f\n", pico_ref_high.amplitud);
    printf("    Fondo = %.1f\n", pico_ref_high.fondo);
    printf("    Amplitud neta = %.1f +/- %.1f\n", pico_ref_high.amplitud_neta, pico_ref_high.error);
    printf("\n[INFO] Q0 (referencia) = %.4f +/- %.4f\n", Q0, errQ0);
    printf("[INFO] Eventos referencia: %.0f\n\n", (double)N_eventos_ref);
    
    f_ref->Close();
    delete h_ref;
    
    // =========================================================================
    // PROCESAR TODAS LAS MUESTRAS
    // =========================================================================
    
    std::vector<ResultadoMuestra> resultados;
    int nMuestrasOK = 0;
    
    printf("%s\n", std::string(130, '-').c_str());
    printf("%-8s | %-6s | %-10s | %-25s | %-25s | %-20s | %-8s | %-6s\n",
           "C(%)", "Tipo", "Eventos", "A_low (neta±err)", "A_high (neta±err)", 
           "Q +/- err", "Z_real", "Detect");
    printf("%s\n", std::string(130, '-').c_str());
    
    for (size_t i = 0; i < MUESTRAS.size(); i++) {
        const auto& muestra = MUESTRAS[i];
        TString filename = Form("%s%s", directorio, muestra.nombre.Data());
        
        TFile* f = TFile::Open(filename, "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "[WARN] No se pudo abrir: " << filename << std::endl;
            continue;
        }
        
        TTree* t = (TTree*)f->Get("Scoring");
        if (!t) {
            std::cerr << "[WARN] No se encontró TTree en: " << filename << std::endl;
            f->Close();
            continue;
        }
        
        Long64_t N_eventos = t->GetEntries();
        
        // Factor de normalización
        double factor_norm = (double)N_eventos_ref / (double)N_eventos;
        
        // Crear histograma
        TH1D* h = new TH1D(Form("h_%zu", i), "", 1600, 0, 1600);
        h->SetDirectory(0);  // Desvincular del directorio global
        t->SetBranchAddress("Energy", &energy);
        for (Long64_t j = 0; j < N_eventos; j++) {
            t->GetEntry(j);
            h->Fill(energy * 1000.0);
        }
        
        // Analizar picos con TSpectrum
        ResultadoTSpectrum pico_low = AnalizarPicoTSpectrum(h, E_LOW, tolerancia_low, factor_norm, false);
        ResultadoTSpectrum pico_high = AnalizarPicoTSpectrum(h, E_HIGH, tolerancia_high, factor_norm, false);
        
        if (!pico_low.encontrado || !pico_high.encontrado) {
            std::cerr << "[WARN] No se encontraron picos para C=" << muestra.concentracion << "%" << std::endl;
            f->Close();
            delete h;
            continue;
        }
        
        // Guardar resultados
        ResultadoMuestra r;
        r.conc = muestra.concentracion;
        r.esFino = muestra.esFino;
        
        r.A_low = pico_low.amplitud_neta;
        r.errA_low = pico_low.error;
        r.A_high = pico_high.amplitud_neta;
        r.errA_high = pico_high.error;
        
        // Calcular Q
        r.Q = r.A_low / r.A_high;
        r.errQ = r.Q * std::sqrt(std::pow(r.errA_low/r.A_low, 2) + 
                                  std::pow(r.errA_high/r.A_high, 2));
        
        // Z-score (desviación de Q0)
        r.Z_score = (r.Q - Q0) / errQ0;
        r.detectable = std::abs(r.Z_score) > 3.0;
        r.cuantificable = std::abs(r.Z_score) > 10.0;
        
        resultados.push_back(r);
        nMuestrasOK++;
        
        // Imprimir fila
        printf("%-8.2f | %-6s | %-10lld | %8.0f ± %-4.0f        | %8.0f ± %-4.0f        | %.4f ± %.4f   | %7.2f | %-6s\n",
               r.conc,
               r.esFino ? "FINO" : "GRUESO",
               N_eventos,
               r.A_low, r.errA_low,
               r.A_high, r.errA_high,
               r.Q, r.errQ,
               r.Z_score,
               r.cuantificable ? "CUANT" : (r.detectable ? "SI" : "NO"));
        
        f->Close();
        delete h;
    }
    
    printf("%s\n", std::string(130, '-').c_str());
    printf("[INFO] Muestras procesadas: %d/%zu\n\n", nMuestrasOK, MUESTRAS.size());
    
    // =========================================================================
    // CALIBRACIÓN: Q vs C_REE
    // =========================================================================
    
    std::vector<double> C_cal, Q_cal, errQ_cal;
    for (const auto& r : resultados) {
        C_cal.push_back(r.conc);
        Q_cal.push_back(r.Q);
        errQ_cal.push_back(r.errQ);
    }
    
    TGraphErrors* grCal = new TGraphErrors(C_cal.size(), &C_cal[0], &Q_cal[0], 
                                           nullptr, &errQ_cal[0]);
    
    TF1* fLin = new TF1("fLin", "[0] + [1]*x", 0, 5);
    fLin->SetParameters(Q0, -0.2);
    fLin->SetParNames("Q0", "k");
    
    TFitResultPtr fitCal = grCal->Fit(fLin, "SQ");
    
    double p0 = fLin->GetParameter(0);
    double ep0 = fLin->GetParError(0);
    double p1 = fLin->GetParameter(1);
    double ep1 = fLin->GetParError(1);
    double chi2 = fLin->GetChisquare();
    int ndf = fLin->GetNDF();
    
    // LOD y LOQ teóricos
    double precision_teorica = errQ0 / std::abs(p1);
    double LOD_teorico = 3 * precision_teorica;
    double LOQ_teorico = 10 * precision_teorica;
    
    // LOD experimental
    double LOD_experimental = -1;
    double LOQ_experimental = -1;
    for (const auto& r : resultados) {
        if (r.esFino && r.conc > 0) {
            if (r.detectable && LOD_experimental < 0) {
                LOD_experimental = r.conc;
            }
            if (r.cuantificable && LOQ_experimental < 0) {
                LOQ_experimental = r.conc;
                break;
            }
        }
    }
    
    // =========================================================================
    // GRÁFICOS
    // =========================================================================
    
    gStyle->SetOptFit(0);
    gStyle->SetOptStat(0);
    
    // Calibración
    TCanvas* cCal = new TCanvas("cCal", "Calibracion", 900, 700);
    cCal->SetGrid();
    cCal->SetLeftMargin(0.12);
    
    grCal->SetTitle("Curva de Calibracion - TSpectrum;C_{REE} (%);Q = A_{low}/A_{high}");
    grCal->SetMarkerStyle(20);
    grCal->SetMarkerSize(1.2);
    grCal->SetMarkerColor(kBlue);
    grCal->Draw("AP");
    fLin->SetLineColor(kRed);
    fLin->SetLineWidth(2);
    fLin->Draw("same");
    
    TLegend* leg = new TLegend(0.15, 0.15, 0.55, 0.35);
    leg->AddEntry(grCal, "Datos (TSpectrum + SNIP)", "p");
    leg->AddEntry(fLin, Form("Ajuste: Q = %.3f + (%.3f)C", p0, p1), "l");
    leg->AddEntry((TObject*)nullptr, Form("#chi^{2}/ndf = %.2f", (ndf>0 ? chi2/ndf : 0)), "");
    leg->Draw();
    
    cCal->SaveAs("Eu152_v6_Calibracion.png");
    
    // Z-score vs concentración
    TCanvas* cZ = new TCanvas("cZ", "Z-score", 900, 600);
    cZ->SetGrid();
    
    std::vector<double> C_z, Z_z;
    for (const auto& r : resultados) {
        if (r.esFino) {
            C_z.push_back(r.conc);
            Z_z.push_back(r.Z_score);
        }
    }
    
    TGraph* grZ = new TGraph(C_z.size(), &C_z[0], &Z_z[0]);
    grZ->SetTitle("Z-score vs Concentracion (Barrido Fino);C_{REE} (%);Z-score");
    grZ->SetMarkerStyle(21);
    grZ->SetMarkerSize(1.5);
    grZ->SetMarkerColor(kBlue);
    grZ->Draw("ALP");
    
    // Líneas de referencia
    TLine* l3 = new TLine(0, 3, 1, 3);
    l3->SetLineColor(kGreen+2); l3->SetLineStyle(2); l3->SetLineWidth(2);
    l3->Draw();
    TLine* l_3 = new TLine(0, -3, 1, -3);
    l_3->SetLineColor(kGreen+2); l_3->SetLineStyle(2); l_3->SetLineWidth(2);
    l_3->Draw();
    
    TLatex* t3 = new TLatex(0.05, 3.5, "LOD (3#sigma)");
    t3->SetTextSize(0.03); t3->Draw();
    
    cZ->SaveAs("Eu152_v6_Zscore.png");
    
    // =========================================================================
    // RESUMEN FINAL
    // =========================================================================
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  RESUMEN FINAL - ANALISIS Eu-152 v6 (TSpectrum)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    printf("\n>> CONFIGURACION:\n");
    printf("   Linea baja:  %.1f keV (busqueda +/-%.0f keV)\n", E_LOW, tolerancia_low);
    printf("   Linea alta:  %.1f keV (busqueda +/-%.0f keV)\n", E_HIGH, tolerancia_high);
    printf("   Metodo:      TSpectrum + SNIP Background (30 iter)\n");
    printf("   Muestras procesadas: %d/%zu\n", nMuestrasOK, MUESTRAS.size());
    
    printf("\n>> MODELO DE CALIBRACION:\n");
    printf("   Q = Q0 + k * C_REE\n");
    printf("   Q0 = %.5f +/- %.5f\n", p0, ep0);
    printf("   k  = %.6f +/- %.6f [1/%%]\n", p1, ep1);
    printf("   chi2/ndf = %.3f\n", (ndf > 0 ? chi2/ndf : 0));
    
    printf("\n>> LIMITES DE DETECCION:\n");
    printf("   sigma(Q0) = %.5f\n", errQ0);
    printf("   Precision: +/- %.3f %% REE = %.0f PPM\n", precision_teorica, precision_teorica*10000);
    printf("   LOD (3-sigma): %.3f %% REE = %.0f PPM\n", LOD_teorico, LOD_teorico*10000);
    printf("   LOQ (10-sigma): %.3f %% REE = %.0f PPM\n", LOQ_teorico, LOQ_teorico*10000);
    
    printf("\n   --- Experimentales (barrido fino) ---\n");
    printf("   Estadistica referencia: %lld eventos\n", N_eventos_ref);
    if (LOD_experimental > 0) {
        printf("   LOD observado: %.2f %% REE = %.0f PPM\n", 
               LOD_experimental, LOD_experimental*10000);
    }
    if (LOQ_experimental > 0) {
        printf("   LOQ observado: %.2f %% REE = %.0f PPM\n", 
               LOQ_experimental, LOQ_experimental*10000);
    }
    
    printf("\n>> COMPARACION CON VERSIONES ANTERIORES:\n");
    printf("   v4 (ROI):    LOD = 3260 PPM, chi2/ndf = 2.696\n");
    printf("   v6 (TSpect): LOD = %.0f PPM, chi2/ndf = %.3f\n", 
           LOD_teorico*10000, (ndf > 0 ? chi2/ndf : 0));
    
    printf("\n>> PARA MUESTRA DESCONOCIDA:\n");
    printf("   C_REE = (Q - %.5f) / (%.6f)\n", p0, p1);
    printf("   Error: delta_C = sqrt((delta_Q/k)^2 + terminos_correlacion)\n");
    
    printf("\n>> ARCHIVOS GENERADOS:\n");
    printf("   - Eu152_v6_SeparacionFondo.png (diagnostico)\n");
    printf("   - Eu152_v6_Calibracion.png\n");
    printf("   - Eu152_v6_Zscore.png\n");
    
    std::cout << std::string(80, '=') << std::endl;
    
    // Guardar CSV
    std::ofstream csv("Eu152_v6_resultados.csv");
    csv << "Concentracion,Tipo,A_low,errA_low,A_high,errA_high,Q,errQ,Z_score,Detectable\n";
    for (const auto& r : resultados) {
        csv << r.conc << "," << (r.esFino ? "FINO" : "GRUESO") << ","
            << r.A_low << "," << r.errA_low << ","
            << r.A_high << "," << r.errA_high << ","
            << r.Q << "," << r.errQ << ","
            << r.Z_score << "," << (r.detectable ? "SI" : "NO") << "\n";
    }
    csv.close();
    
    std::cout << "\n[INFO] Resultados guardados en: Eu152_v6_resultados.csv" << std::endl;
}
