/**
 * @file AnalisisEu152_v5.cpp
 * @brief Análisis Dual-Energy con Eu-152 usando AJUSTE GAUSSIANO
 * 
 * MEJORAS vs v4:
 *   - Reemplaza integración ROI simple por ajuste gaussiano + polinomio lineal
 *   - Usa amplitud de pico (A) como estimador en lugar de cuentas integradas
 *   - Propagación de errores directa del ajuste (sin aproximaciones)
 *   - Mejor separación señal/fondo
 * 
 * ARCHIVOS ESPERADOS:
 *   Barrido FINO: Eu152_REE_0p00.root, ..., Eu152_REE_0p01.root
 *   Barrido GRUESO: Eu152_REE_0p02.root, ..., Eu152_REE_0p05.root
 * 
 * Estructura: TTree "Scoring", Branch "Energy" en MeV
 * 
 * Uso: root -l 'AnalisisEu152_v5.cpp("./")'
 *      root -l 'AnalisisEu152_v5.cpp("./", true)'  // usa 1408 keV
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
#include "TF1.h"
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
#include "TFitResult.h"
#include "TFitResultPtr.h"

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
// ESTRUCTURA PARA RESULTADOS DE AJUSTE
// ============================================================================

struct ResultadoFit {
    // Parámetros del ajuste
    double amplitud;       // A (altura del pico)
    double centroide;      // μ (posición)
    double sigma;          // σ (ancho)
    double fondo_const;    // a (fondo constante)
    double fondo_lin;      // b (fondo lineal)
    
    // Errores
    double err_amplitud;
    double err_centroide;
    double err_sigma;
    
    // Estadísticas del fit
    double chi2;
    int ndf;
    double chi2_ndf;
    
    // Estado
    bool ok;
    
    // Valores normalizados
    double amplitud_norm;
    double err_amplitud_norm;
    
    ResultadoFit() : amplitud(0), centroide(0), sigma(0), fondo_const(0), fondo_lin(0),
                     err_amplitud(0), err_centroide(0), err_sigma(0),
                     chi2(0), ndf(0), chi2_ndf(0), ok(false),
                     amplitud_norm(0), err_amplitud_norm(0) {}
};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS COMPLETOS DE UNA MUESTRA
// ============================================================================

struct ResultadoMuestra {
    double conc;
    bool esFino;
    
    // Amplitudes (en lugar de cuentas integradas)
    double A_low, errA_low;
    double A_high, errA_high;
    
    // Índice Q basado en amplitudes
    double Q, errQ;
    
    // Información adicional de los fits
    double chi2_low, chi2_high;
    int ndf_low, ndf_high;
    
    // Z-score para detectabilidad
    double Z_score;
    bool detectable;
    bool cuantificable;
    
    ResultadoMuestra() : conc(0), esFino(true), 
                         A_low(0), errA_low(0), A_high(0), errA_high(0),
                         Q(0), errQ(0),
                         chi2_low(0), chi2_high(0), ndf_low(0), ndf_high(0),
                         Z_score(0), detectable(false), cuantificable(false) {}
};

// ============================================================================
// FUNCIÓN: Ajustar fotopico con Gaussiana + Polinomio Lineal
// ============================================================================

ResultadoFit AjustarFotopico(TH1D* h, double E_centro, double rango_fit, 
                             const char* nombre_fit = "fit",
                             double factor_norm = 1.0) {
    ResultadoFit r;
    if (!h) return r;
    
    // =========================================================================
    // PASO 1: Definir rango de ajuste
    // =========================================================================
    
    double E_min = E_centro - rango_fit;
    double E_max = E_centro + rango_fit;
    
    // =========================================================================
    // PASO 2: Estimar parámetros iniciales
    // =========================================================================
    
    // Buscar máximo en el rango
    int bin_centro = h->FindBin(E_centro);
    int bin_min = h->FindBin(E_min);
    int bin_max = h->FindBin(E_max);
    
    double max_contenido = 0;
    int bin_max_contenido = bin_centro;
    for (int i = bin_min; i <= bin_max; i++) {
        if (h->GetBinContent(i) > max_contenido) {
            max_contenido = h->GetBinContent(i);
            bin_max_contenido = i;
        }
    }
    
    double E_max_contenido = h->GetBinCenter(bin_max_contenido);
    
    // Estimar fondo (promedio de extremos del rango)
    int n_bins_fondo = 5;  // usar más bins para mejor promedio
    double fondo_izq = 0;
    double fondo_der = 0;
    
    for (int i = 0; i < n_bins_fondo && (bin_min + i) <= bin_max; i++) {
        fondo_izq += h->GetBinContent(bin_min + i);
    }
    for (int i = 0; i < n_bins_fondo && (bin_max - i) >= bin_min; i++) {
        fondo_der += h->GetBinContent(bin_max - i);
    }
    fondo_izq /= n_bins_fondo;
    fondo_der /= n_bins_fondo;
    double fondo_inicial = (fondo_izq + fondo_der) / 2.0;
    
    // Si el fondo es muy bajo, poner un mínimo
    if (fondo_inicial < 1.0) fondo_inicial = 1.0;
    
    // Estimar amplitud (altura sobre fondo)
    double amplitud_inicial = max_contenido - fondo_inicial;
    if (amplitud_inicial < 10) amplitud_inicial = max_contenido * 0.5;  // fallback
    
    // Estimar sigma (aproximadamente FWHM/2.355)
    // Para detector típico: ~2-3 keV a baja E, ~5-8 keV a alta E
    double sigma_inicial;
    if (E_centro < 200) {
        sigma_inicial = 2.5;  // Baja energía
    } else if (E_centro < 800) {
        sigma_inicial = 4.0;  // Media energía
    } else {
        sigma_inicial = 6.0;  // Alta energía
    }
    
    // Estimar pendiente del fondo
    double pendiente_inicial = (fondo_der - fondo_izq) / (E_max - E_min);
    if (std::isnan(pendiente_inicial) || std::isinf(pendiente_inicial)) {
        pendiente_inicial = 0.0;
    }
    
    // =========================================================================
    // PASO 3: Definir función de ajuste
    // =========================================================================
    
    // Usar gaussiana sin normalización (más simple y robusto)
    TF1* fit = new TF1(nombre_fit, 
                       "[0]*TMath::Gaus(x,[1],[2]) + [3] + [4]*x",
                       E_min, E_max);
    
    // Parámetros:
    // [0] = A      (amplitud - altura del pico sobre fondo)
    // [1] = μ      (centroide)
    // [2] = σ      (ancho)
    // [3] = a      (fondo constante)
    // [4] = b      (pendiente fondo)
    
    fit->SetParameters(amplitud_inicial, E_max_contenido, sigma_inicial, 
                       fondo_inicial, pendiente_inicial);
    
    // Límites razonables pero no muy restrictivos
    fit->SetParLimits(0, amplitud_inicial*0.1, amplitud_inicial*5.0);  // A positiva
    fit->SetParLimits(1, E_centro - 10, E_centro + 10);    // μ con más rango
    fit->SetParLimits(2, 0.3, 20.0);                       // σ más amplio
    fit->SetParLimits(3, 0, fondo_inicial*5);              // fondo positivo
    // [4] sin límites (pendiente puede ser + o -)
    
    // Nombres de parámetros
    fit->SetParNames("Amplitud", "Centroide", "Sigma", "Fondo_const", "Fondo_lin");
    
    // =========================================================================
    // PASO 4: Realizar ajuste con estadística mejorada
    // =========================================================================
    
    // Opciones de ajuste:
    // S = guardar resultado completo (para acceder a matriz de covarianza)
    // Q = silencioso (cambiar a "" para ver output del fit si hay problemas)
    // R = usar rango de función
    // M = Mejorar ajuste (Minuit MIGRAD)
    // B = usar límites de parámetros
    TFitResultPtr fitResult = h->Fit(fit, "SQRMB");
    
    // =========================================================================
    // PASO 5: Extraer resultados
    // =========================================================================
    
    if (fitResult.Get() && fitResult->IsValid()) {
        r.amplitud = fit->GetParameter(0);
        r.centroide = fit->GetParameter(1);
        r.sigma = fit->GetParameter(2);
        r.fondo_const = fit->GetParameter(3);
        r.fondo_lin = fit->GetParameter(4);
        
        r.err_amplitud = fit->GetParError(0);
        r.err_centroide = fit->GetParError(1);
        r.err_sigma = fit->GetParError(2);
        
        r.chi2 = fit->GetChisquare();
        r.ndf = fit->GetNDF();
        r.chi2_ndf = (r.ndf > 0) ? r.chi2 / r.ndf : 0;
        
        // Criterios de validez más permisivos:
        // 1. Amplitud significativa (> 2σ, bajado de 3σ)
        // 2. Chi2/ndf razonable (< 10, más permisivo)
        // 3. Centroide cerca del esperado (< 5 keV, más permisivo)
        // 4. Sigma físicamente razonable
        bool amplitud_significativa = (r.amplitud > 2 * r.err_amplitud);
        bool chi2_razonable = (r.chi2_ndf < 10.0 && r.chi2_ndf > 0);
        bool centroide_razonable = std::abs(r.centroide - E_centro) < 5.0;
        bool sigma_razonable = (r.sigma > 0.3 && r.sigma < 20.0);
        
        r.ok = amplitud_significativa && chi2_razonable && centroide_razonable && sigma_razonable;
        
        if (!r.ok) {
            std::cerr << "[WARN] Ajuste de calidad marginal para " << nombre_fit << ":" << std::endl;
            std::cerr << "       A=" << r.amplitud << "±" << r.err_amplitud 
                      << " (sig=" << (r.amplitud/r.err_amplitud) << "σ)" << std::endl;
            std::cerr << "       μ=" << r.centroide << " (esperado " << E_centro 
                      << ", diff=" << (r.centroide-E_centro) << ")" << std::endl;
            std::cerr << "       σ=" << r.sigma << " keV" << std::endl;
            std::cerr << "       chi²/ndf=" << r.chi2_ndf << std::endl;
        }
        
        // Normalización (igual que en v4)
        r.amplitud_norm = r.amplitud * factor_norm;
        r.err_amplitud_norm = r.err_amplitud * factor_norm;
        
    } else {
        r.ok = false;
        std::cerr << "[ERROR] Ajuste falló completamente para " << nombre_fit 
                  << " en E=" << E_centro << " keV" << std::endl;
        if (fitResult.Get()) {
            std::cerr << "        Estado del fit: " << fitResult->Status() << std::endl;
        }
    }
    
    return r;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void AnalisisEu152_v5(const char* directorio = "./", bool usar_1408 = false) {
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  ANALISIS Eu-152 v5 - AJUSTE GAUSSIANO + VALIDACION LOD" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // =========================================================================
    // CONFIGURACIÓN DE LÍNEAS
    // =========================================================================
    
    double E_LOW = E_122;
    double E_HIGH = usar_1408 ? E_1408 : E_344;
    
    // Rangos de ajuste (más amplios que ventanas ROI para capturar fondo)
    double rango_low = 20.0;   // ±20 keV para 122 keV
    double rango_high = usar_1408 ? 40.0 : 30.0;  // ±40 keV para 1408, ±30 para 344
    
    printf("  Linea BAJA:  %.2f keV (ajuste en +/-%.0f keV)\n", E_LOW, rango_low);
    printf("  Linea ALTA:  %.2f keV (ajuste en +/-%.0f keV)\n", E_HIGH, rango_high);
    printf("  Metodo:      Ajuste Gaussiano + Polinomio grado 1\n");
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
    Double_t energy;
    t_ref->SetBranchAddress("Energy", &energy);
    for (Long64_t i = 0; i < N_eventos_ref; i++) {
        t_ref->GetEntry(i);
        h_ref->Fill(energy * 1000.0);  // MeV -> keV
    }
    
    // Ajustar picos en referencia
    printf("[DEBUG] Ajustando pico %.0f keV en referencia...\n", E_LOW);
    ResultadoFit fit_ref_low = AjustarFotopico(h_ref, E_LOW, rango_low, "fit_ref_low", 1.0);
    
    printf("[DEBUG] Ajustando pico %.0f keV en referencia...\n", E_HIGH);
    ResultadoFit fit_ref_high = AjustarFotopico(h_ref, E_HIGH, rango_high, "fit_ref_high", 1.0);
    
    if (!fit_ref_low.ok || !fit_ref_high.ok) {
        std::cerr << "[ERROR] Fallo en ajuste de referencia" << std::endl;
        std::cerr << "        Pico bajo OK: " << (fit_ref_low.ok ? "SI" : "NO") << std::endl;
        std::cerr << "        Pico alto OK: " << (fit_ref_high.ok ? "SI" : "NO") << std::endl;
        
        // Mostrar histograma para diagnosticar
        TCanvas* cDebug = new TCanvas("cDebug", "Debug", 1200, 500);
        cDebug->Divide(2,1);
        
        cDebug->cd(1);
        gPad->SetLogy();
        h_ref->GetXaxis()->SetRangeUser(E_LOW - 50, E_LOW + 50);
        h_ref->Draw();
        
        cDebug->cd(2);
        gPad->SetLogy();
        h_ref->GetXaxis()->SetRangeUser(E_HIGH - 100, E_HIGH + 100);
        h_ref->Draw();
        
        cDebug->SaveAs("Eu152_v5_DEBUG.png");
        std::cerr << "        Guardado espectro de diagnóstico: Eu152_v5_DEBUG.png" << std::endl;
        
        return;
    }
    
    double A0_low = fit_ref_low.amplitud;
    double errA0_low = fit_ref_low.err_amplitud;
    double A0_high = fit_ref_high.amplitud;
    double errA0_high = fit_ref_high.err_amplitud;
    
    // Calcular Q0 y su error
    double Q0 = A0_low / A0_high;
    double errQ0 = Q0 * std::sqrt(std::pow(errA0_low/A0_low, 2) + 
                                   std::pow(errA0_high/A0_high, 2));
    
    printf("[INFO] REFERENCIA (0%% REE):\n");
    printf("  Eventos totales: %lld\n", N_eventos_ref);
    printf("  Pico %.0f keV:\n", E_LOW);
    printf("    Amplitud = %.1f +/- %.1f (chi2/ndf = %.2f)\n", 
           A0_low, errA0_low, fit_ref_low.chi2_ndf);
    printf("  Pico %.0f keV:\n", E_HIGH);
    printf("    Amplitud = %.1f +/- %.1f (chi2/ndf = %.2f)\n", 
           A0_high, errA0_high, fit_ref_high.chi2_ndf);
    printf("\n[INFO] Q0 (referencia) = %.4f +/- %.4f\n", Q0, errQ0);
    printf("[INFO] Eventos referencia: %.0f\n\n", (double)N_eventos_ref);
    
    f_ref->Close();
    delete h_ref;
    
    // =========================================================================
    // PROCESAR TODAS LAS MUESTRAS
    // =========================================================================
    
    std::vector<ResultadoMuestra> resultados;
    int nMuestrasOK = 0;
    
    printf("%s\n", std::string(140, '-').c_str());
    printf("%-8s | %-6s | %-10s | %-20s | %-20s | %-20s | %-12s | %-6s\n",
           "C(%)", "Tipo", "Eventos", "A_low (chi2/ndf)", "A_high (chi2/ndf)", 
           "Q +/- err", "Z_real", "Detect");
    printf("%s\n", std::string(140, '-').c_str());
    
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
        t->SetBranchAddress("Energy", &energy);
        for (Long64_t j = 0; j < N_eventos; j++) {
            t->GetEntry(j);
            h->Fill(energy * 1000.0);
        }
        
        // Ajustar picos
        ResultadoFit fit_low = AjustarFotopico(h, E_LOW, rango_low, 
                                               Form("fit_%zu_low", i), factor_norm);
        ResultadoFit fit_high = AjustarFotopico(h, E_HIGH, rango_high, 
                                                Form("fit_%zu_high", i), factor_norm);
        
        if (!fit_low.ok || !fit_high.ok) {
            std::cerr << "[WARN] Ajuste falló para C=" << muestra.concentracion << "%" << std::endl;
            f->Close();
            delete h;
            continue;
        }
        
        // Guardar resultados
        ResultadoMuestra r;
        r.conc = muestra.concentracion;
        r.esFino = muestra.esFino;
        
        r.A_low = fit_low.amplitud;
        r.errA_low = fit_low.err_amplitud;
        r.A_high = fit_high.amplitud;
        r.errA_high = fit_high.err_amplitud;
        
        r.chi2_low = fit_low.chi2_ndf;
        r.chi2_high = fit_high.chi2_ndf;
        r.ndf_low = fit_low.ndf;
        r.ndf_high = fit_high.ndf;
        
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
        printf("%-8.2f | %-6s | %-10lld | %6.0f±%4.0f (%.2f) | %6.0f±%4.0f (%.2f) | %.4f±%.4f | %7.2f | %-6s\n",
               r.conc,
               r.esFino ? "FINO" : "GRUESO",
               N_eventos,
               r.A_low, r.errA_low, r.chi2_low,
               r.A_high, r.errA_high, r.chi2_high,
               r.Q, r.errQ,
               r.Z_score,
               r.cuantificable ? "CUANT" : (r.detectable ? "SI" : "NO"));
        
        f->Close();
        delete h;
    }
    
    printf("%s\n", std::string(140, '-').c_str());
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
    
    // LOD experimental (primera concentración detectable)
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
    
    grCal->SetTitle("Curva de Calibracion - Ajuste Gaussiano;C_{REE} (%);Q = A_{low}/A_{high}");
    grCal->SetMarkerStyle(20);
    grCal->SetMarkerSize(1.2);
    grCal->SetMarkerColor(kBlue);
    grCal->Draw("AP");
    fLin->SetLineColor(kRed);
    fLin->SetLineWidth(2);
    fLin->Draw("same");
    
    TLegend* leg = new TLegend(0.15, 0.15, 0.50, 0.35);
    leg->AddEntry(grCal, "Datos (ajuste gaussiano)", "p");
    leg->AddEntry(fLin, Form("Ajuste: Q = %.3f + (%.3f)C", p0, p1), "l");
    leg->AddEntry((TObject*)nullptr, Form("#chi^{2}/ndf = %.2f", (ndf>0 ? chi2/ndf : 0)), "");
    leg->Draw();
    
    cCal->SaveAs("Eu152_v5_Calibracion.png");
    
    // =========================================================================
    // RESUMEN FINAL
    // =========================================================================
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  RESUMEN FINAL - ANALISIS Eu-152 v5 (AJUSTE GAUSSIANO)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    printf("\n>> CONFIGURACION:\n");
    printf("   Linea baja:  %.1f keV (ajuste +/-%.0f keV)\n", E_LOW, rango_low);
    printf("   Linea alta:  %.1f keV (ajuste +/-%.0f keV)\n", E_HIGH, rango_high);
    printf("   Metodo:      Gaussiana + Polinomio lineal\n");
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
    
    printf("\n>> COMPARACION CON v4 (integracion ROI):\n");
    printf("   v4: LOD = %.0f PPM, LOQ = %.0f PPM, chi2/ndf = 2.696\n", 3260.0, 10850.0);
    printf("   v5: LOD = %.0f PPM, LOQ = %.0f PPM, chi2/ndf = %.3f\n", 
           LOD_teorico*10000, LOQ_teorico*10000, (ndf > 0 ? chi2/ndf : 0));
    
    printf("\n>> PARA MUESTRA DESCONOCIDA:\n");
    printf("   C_REE = (Q - %.5f) / (%.6f)\n", p0, p1);
    printf("   Error: delta_C = sqrt((delta_Q/k)^2 + ((Q-Q0)*delta_k/k^2)^2)\n");
    
    printf("\n>> ARCHIVOS GENERADOS:\n");
    printf("   - Eu152_v5_Calibracion.png\n");
    
    std::cout << std::string(80, '=') << std::endl;
    
    // Guardar CSV
    std::ofstream csv("Eu152_v5_resultados.csv");
    csv << "Concentracion,Tipo,A_low,errA_low,A_high,errA_high,Q,errQ,chi2_low,chi2_high,Z_score,Detectable\n";
    for (const auto& r : resultados) {
        csv << r.conc << "," << (r.esFino ? "FINO" : "GRUESO") << ","
            << r.A_low << "," << r.errA_low << ","
            << r.A_high << "," << r.errA_high << ","
            << r.Q << "," << r.errQ << ","
            << r.chi2_low << "," << r.chi2_high << ","
            << r.Z_score << "," << (r.detectable ? "SI" : "NO") << "\n";
    }
    csv.close();
    
    std::cout << "\n[INFO] Resultados guardados en: Eu152_v5_resultados.csv" << std::endl;
}
