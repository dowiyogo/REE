/**
 * @file AnalisisEu152_v4.cpp
 * @brief Análisis Dual-Energy con Eu-152 para validación de LOD en REE
 * 
 * ARCHIVOS ESPERADOS:
 *   Barrido FINO (validación LOD):
 *     Eu152_REE_0p00.root, Eu152_REE_0p002.root, ..., Eu152_REE_0p01.root
 *   Barrido GRUESO (calibración):
 *     Eu152_REE_0p02.root, Eu152_REE_0p03.root, ..., Eu152_REE_0p05.root
 * 
 * Estructura: TTree "Scoring", Branch "Energy" en MeV
 * 
 * OBJETIVO:
 *   - Validar límite de detección (LOD) con datos de barrido fino
 *   - Generar curva de calibración con todos los datos
 *   - Determinar si concentraciones bajas son estadísticamente detectables
 * 
 * Uso: root -l 'AnalisisEu152_v4.cpp("./")'
 *      root -l 'AnalisisEu152_v4.cpp("./", true)'  // usa 1408 keV
 * 
 * @author Claude/Usuario
 * @date 2025
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

// ============================================================================
// CONFIGURACIÓN - LÍNEAS Eu-152
// ============================================================================

// Energías de los fotopicos (keV)
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
    TString nombre;           // Nombre del archivo (sin path)
    double concentracion;     // Concentración REE (%)
    bool esFino;             // true = barrido fino, false = grueso
};

// Lista de todas las muestras a procesar
const std::vector<MuestraInfo> MUESTRAS = {
    // Barrido FINO (validación LOD) - concentraciones muy bajas
    {"Eu152_REE_0p00.root",   0.00,  true},   // Referencia
    {"Eu152_REE_0p002.root",  0.20,  true},   // 0.2%
    {"Eu152_REE_0p004.root",  0.40,  true},   // 0.4%
    {"Eu152_REE_0p006.root",  0.60,  true},   // 0.6%
    {"Eu152_REE_0p008.root",  0.80,  true},   // 0.8%
    {"Eu152_REE_0p01.root",   1.00,  true},   // 1.0%
    // Barrido GRUESO (calibración)
    {"Eu152_REE_0p02.root",   2.00,  false},  // 2.0%
    {"Eu152_REE_0p03.root",   3.00,  false},  // 3.0%
    {"Eu152_REE_0p04.root",   4.00,  false},  // 4.0%
    {"Eu152_REE_0p05.root",   5.00,  false},  // 5.0%
};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS DE PICO
// ============================================================================

struct Pico {
    double cuentas_brutas;
    double cuentas_netas;
    double error;
    double fondo;
    bool ok;
    
    // Valores normalizados (al mismo número de eventos)
    double netas_norm;
    double error_norm;
    
    Pico() : cuentas_brutas(0), cuentas_netas(0), error(0), fondo(0), ok(false),
             netas_norm(0), error_norm(0) {}
};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS COMPLETOS DE UNA MUESTRA
// ============================================================================

struct ResultadoMuestra {
    double conc;              // Concentración (%)
    bool esFino;              // Tipo de barrido
    
    // Cuentas
    double N_low, errN_low;
    double N_high, errN_high;
    
    // Transmisiones
    double T_low, errT_low;
    double T_high, errT_high;
    
    // Atenuaciones L = -ln(T)
    double L_low, errL_low;
    double L_high, errL_high;
    
    // Índices
    double R, errR;           // R = L_low / L_high
    double Q, errQ;           // Q = N_low / N_high
    double Delta, errDelta;   // Delta = L_low - L_high
    
    // Z-score para detectabilidad
    double Z_score;           // (Q - Q0) / sigma_Q
    bool detectable;          // |Z| > 3 (3-sigma)
    bool cuantificable;       // |Z| > 10 (10-sigma)
    
    ResultadoMuestra() : conc(0), esFino(true), N_low(0), errN_low(0), N_high(0), errN_high(0),
                         T_low(0), errT_low(0), T_high(0), errT_high(0),
                         L_low(0), errL_low(0), L_high(0), errL_high(0),
                         R(0), errR(0), Q(0), errQ(0), Delta(0), errDelta(0),
                         Z_score(0), detectable(false), cuantificable(false) {}
};

// ============================================================================
// FUNCIÓN: Integrar fotopico con sustracción de fondo
// Ahora incluye normalización opcional
// ============================================================================

Pico IntegrarFotopico(TH1D* h, double E_centro, double semi_ancho, 
                      double factor_norm = 1.0) {
    Pico p;
    if (!h) return p;
    
    // ROI del pico
    int bin_pico_min = h->FindBin(E_centro - semi_ancho);
    int bin_pico_max = h->FindBin(E_centro + semi_ancho);
    
    p.cuentas_brutas = h->Integral(bin_pico_min, bin_pico_max);
    
    // Regiones de fondo (bandas laterales)
    int bin_bg_L1 = h->FindBin(E_centro - 2*semi_ancho);
    int bin_bg_L2 = h->FindBin(E_centro - semi_ancho) - 1;
    int bin_bg_R1 = h->FindBin(E_centro + semi_ancho) + 1;
    int bin_bg_R2 = h->FindBin(E_centro + 2*semi_ancho);
    
    double bg_izq = h->Integral(bin_bg_L1, bin_bg_L2);
    double bg_der = h->Integral(bin_bg_R1, bin_bg_R2);
    
    int n_pico = bin_pico_max - bin_pico_min + 1;
    int n_bg_izq = bin_bg_L2 - bin_bg_L1 + 1;
    int n_bg_der = bin_bg_R2 - bin_bg_R1 + 1;
    int n_bg_total = n_bg_izq + n_bg_der;
    
    double bg_total = bg_izq + bg_der;
    p.fondo = bg_total * (double)n_pico / (double)n_bg_total;
    
    p.cuentas_netas = p.cuentas_brutas - p.fondo;
    
    // Error estadístico ORIGINAL (basado en cuentas reales)
    p.error = std::sqrt(p.cuentas_brutas + p.fondo);
    
    // Valores NORMALIZADOS
    // Si factor_norm > 1, significa que esta muestra tiene menos estadística
    // y escalamos las cuentas para hacerlas comparables
    p.netas_norm = p.cuentas_netas * factor_norm;
    
    // El error normalizado: si escalamos por k, el error escala como sqrt(k)*sigma
    // Porque el error relativo se mantiene: sigma_norm/N_norm = sigma/N
    // Entonces: sigma_norm = factor_norm * sigma
    // PERO queremos el error que TENDRÍAMOS si hubiéramos medido con más estadística
    // Eso sería: sigma_norm = sqrt(N_norm) = sqrt(factor_norm * N) = sqrt(factor_norm) * sqrt(N)
    // No, mejor: mantenemos el error relativo = sigma/N
    // sigma_norm = (sigma/N) * N_norm = (sigma/N) * N * factor_norm = sigma * factor_norm
    p.error_norm = p.error * factor_norm;
    
    p.ok = (p.cuentas_netas > 0 && p.cuentas_netas > 3 * p.error);
    
    return p;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void AnalisisEu152_v4(const char* directorio = "./", bool usar_1408 = false) {
    
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(1111);
    gROOT->SetBatch(kFALSE);
    
    // =========================================================================
    // Configuración de líneas espectrales
    // =========================================================================
    double E_LOW = E_122;
    double E_HIGH = usar_1408 ? E_1408 : E_779;
    double ventana_low = 12.0;
    double ventana_high = usar_1408 ? 25.0 : 20.0;
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  ANALISIS Eu-152 v4 - VALIDACION DE LOD Y CALIBRACION" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "  Linea BAJA:  " << E_LOW << " keV (fotoelectrico, sensible a Z)" << std::endl;
    std::cout << "  Linea ALTA:  " << E_HIGH << " keV (Compton, sensible a densidad)" << std::endl;
    std::cout << "  Ventanas:    +/-" << ventana_low << " keV, +/-" << ventana_high << " keV" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // =========================================================================
    // PASO 1: Cargar REFERENCIA (0% REE)
    // =========================================================================
    
    TString filename_ref = Form("%s%s", directorio, MUESTRAS[0].nombre.Data());
    TFile* f_ref = TFile::Open(filename_ref);
    if (!f_ref || f_ref->IsZombie()) {
        std::cerr << "[ERROR] No se puede abrir referencia: " << filename_ref << std::endl;
        return;
    }
    
    TTree* t_ref = (TTree*)f_ref->Get("Scoring");
    if (!t_ref) {
        std::cerr << "[ERROR] TTree 'Scoring' no encontrado en referencia" << std::endl;
        return;
    }
    
    TH1D* h_ref = new TH1D("h_ref", "Referencia 0% REE", 1600, 0, 1600);
    t_ref->Draw("Energy*1000>>h_ref", "", "goff");
    
    Pico ref_low = IntegrarFotopico(h_ref, E_LOW, ventana_low);
    Pico ref_high = IntegrarFotopico(h_ref, E_HIGH, ventana_high);
    
    std::cout << "\n[INFO] REFERENCIA (0% REE):" << std::endl;
    printf("  Eventos totales: %.0f\n", h_ref->GetEntries());
    printf("  %.0f keV: Netas = %.0f +/- %.0f\n", E_LOW, ref_low.cuentas_netas, ref_low.error);
    printf("  %.0f keV: Netas = %.0f +/- %.0f\n", E_HIGH, ref_high.cuentas_netas, ref_high.error);
    
    if (ref_low.cuentas_netas <= 0 || ref_high.cuentas_netas <= 0) {
        std::cerr << "[ERROR] Cuentas netas <= 0 en referencia" << std::endl;
        return;
    }
    
    // Q0 de referencia
    double Q0 = ref_low.cuentas_netas / ref_high.cuentas_netas;
    double errQ0 = Q0 * std::sqrt(std::pow(ref_low.error/ref_low.cuentas_netas, 2) +
                                   std::pow(ref_high.error/ref_high.cuentas_netas, 2));
    
    std::cout << "\n[INFO] Q0 (referencia) = " << Q0 << " +/- " << errQ0 << std::endl;
    
    // Número de eventos de referencia (para normalización)
    double N_eventos_ref = h_ref->GetEntries();
    std::cout << "[INFO] Eventos referencia (para normalizar): " << N_eventos_ref << std::endl;
    
    // =========================================================================
    // PASO 2: Procesar TODAS las muestras (con normalización)
    // =========================================================================
    
    std::vector<ResultadoMuestra> resultados;
    int nMuestrasOK = 0;
    
    std::cout << "\n" << std::string(140, '-') << std::endl;
    printf("%-8s | %-6s | %-8s | %-10s | %-10s | %-12s | %-12s | %-7s | %-7s | %-6s\n",
           "C(%)", "Tipo", "Eventos", "N_low", "N_high", "Q +/- err", "Q_norm+/-err", "Z_real", "Z_norm", "Detect");
    std::cout << std::string(140, '-') << std::endl;
    
    for (size_t i = 0; i < MUESTRAS.size(); i++) {
        TString filename = Form("%s%s", directorio, MUESTRAS[i].nombre.Data());
        TFile* f = TFile::Open(filename, "READ");
        
        if (!f || f->IsZombie()) {
            std::cerr << "[WARN] No se puede abrir: " << filename << std::endl;
            continue;
        }
        
        TTree* t = (TTree*)f->Get("Scoring");
        if (!t) {
            f->Close();
            continue;
        }
        
        TString hname = Form("h_%zu", i);
        TH1D* h = new TH1D(hname, Form("%.2f%% REE", MUESTRAS[i].concentracion), 1600, 0, 1600);
        t->Draw(Form("Energy*1000>>%s", hname.Data()), "", "goff");
        
        double N_eventos = h->GetEntries();
        
        // Factor de normalización: escalar al mismo número de eventos que la referencia
        double factor_norm = N_eventos_ref / N_eventos;
        
        Pico p_low = IntegrarFotopico(h, E_LOW, ventana_low, factor_norm);
        Pico p_high = IntegrarFotopico(h, E_HIGH, ventana_high, factor_norm);
        
        // Crear resultado
        ResultadoMuestra res;
        res.conc = MUESTRAS[i].concentracion;
        res.esFino = MUESTRAS[i].esFino;
        
        res.N_low = p_low.cuentas_netas;
        res.errN_low = p_low.error;
        res.N_high = p_high.cuentas_netas;
        res.errN_high = p_high.error;
        
        // Transmisiones (usando valores normalizados para comparación justa)
        res.T_low = p_low.netas_norm / ref_low.cuentas_netas;
        res.T_high = p_high.netas_norm / ref_high.cuentas_netas;
        
        // Errores de transmisión normalizados
        res.errT_low = res.T_low * std::sqrt(
            std::pow(p_low.error_norm/p_low.netas_norm, 2) +
            std::pow(ref_low.error/ref_low.cuentas_netas, 2));
        res.errT_high = res.T_high * std::sqrt(
            std::pow(p_high.error_norm/p_high.netas_norm, 2) +
            std::pow(ref_high.error/ref_high.cuentas_netas, 2));
        
        // Atenuaciones
        if (res.T_low > 0) res.L_low = -std::log(res.T_low);
        if (res.T_high > 0) res.L_high = -std::log(res.T_high);
        res.errL_low = res.errT_low / res.T_low;
        res.errL_high = res.errT_high / res.T_high;
        
        // =====================================================================
        // Índice Q = N_low / N_high (INVARIANTE ante normalización)
        // El ratio es el mismo con o sin normalización
        // =====================================================================
        res.Q = p_low.cuentas_netas / p_high.cuentas_netas;
        
        // Error usando cuentas ORIGINALES (error real de la medición)
        double rel_err_low = p_low.error / p_low.cuentas_netas;
        double rel_err_high = p_high.error / p_high.cuentas_netas;
        res.errQ = res.Q * std::sqrt(rel_err_low*rel_err_low + rel_err_high*rel_err_high);
        
        // =====================================================================
        // Error NORMALIZADO: ¿Qué error tendríamos si midiéramos con N_ref eventos?
        // 
        // El error estadístico escala como: sigma ∝ sqrt(N)
        // El error RELATIVO escala como: sigma/N ∝ 1/sqrt(N)
        // 
        // Si tenemos N eventos y queremos el error "como si" tuviéramos N_ref:
        //   sigma_rel_norm = sigma_rel * sqrt(N / N_ref)
        // 
        // Esto es MENOR si N < N_ref (la referencia tiene más estadística)
        // =====================================================================
        double ratio_estadistica = std::sqrt(N_eventos / N_eventos_ref);
        double rel_err_low_norm = rel_err_low * ratio_estadistica;
        double rel_err_high_norm = rel_err_high * ratio_estadistica;
        double errQ_norm = res.Q * std::sqrt(rel_err_low_norm*rel_err_low_norm + 
                                              rel_err_high_norm*rel_err_high_norm);
        
        // Índice R = L_low / L_high
        if (res.L_high > 0.001) {
            res.R = res.L_low / res.L_high;
            double I = p_low.netas_norm, dI = p_low.error_norm;
            double I0 = ref_low.cuentas_netas, dI0 = ref_low.error;
            double J = p_high.netas_norm, dJ = p_high.error_norm;
            double J0 = ref_high.cuentas_netas, dJ0 = ref_high.error;
            
            double dR_dI = -1.0 / (I * res.L_high);
            double dR_dI0 = 1.0 / (I0 * res.L_high);
            double dR_dJ = res.R / (J * res.L_high);
            double dR_dJ0 = -res.R / (J0 * res.L_high);
            
            res.errR = std::sqrt(std::pow(dR_dI * dI, 2) + std::pow(dR_dI0 * dI0, 2) +
                                 std::pow(dR_dJ * dJ, 2) + std::pow(dR_dJ0 * dJ0, 2));
        }
        
        // Delta = L_low - L_high
        res.Delta = res.L_low - res.L_high;
        res.errDelta = std::sqrt(res.errL_low*res.errL_low + res.errL_high*res.errL_high);
        
        // =====================================================================
        // Z-scores: DOS versiones para comparación
        // =====================================================================
        
        // Z-score REAL: con los errores de la medición actual
        double sigma_real = std::sqrt(res.errQ*res.errQ + errQ0*errQ0);
        double Z_real = (sigma_real > 0) ? (res.Q - Q0) / sigma_real : 0;
        
        // Z-score NORMALIZADO: como si todas las mediciones tuvieran N_ref eventos
        // Esto muestra el POTENCIAL del método con estadística uniforme
        double sigma_norm = std::sqrt(errQ_norm*errQ_norm + errQ0*errQ0);
        double Z_norm = (sigma_norm > 0) ? (res.Q - Q0) / sigma_norm : 0;
        
        // Guardar el Z-score real (conservador)
        res.Z_score = Z_real;
        res.detectable = std::abs(Z_real) > 3.0;
        res.cuantificable = std::abs(Z_real) > 10.0;
        
        resultados.push_back(res);
        nMuestrasOK++;
        
        const char* tipo = res.esFino ? "FINO" : "GRUESO";
        const char* det = res.detectable ? (res.cuantificable ? "CUANT" : "SI") : "NO";
        
        // Mostrar ambos Z-scores
        printf("%-8.2f | %-6s | %-8.0f | %-10.0f | %-10.0f | %.4f+/-%.4f | %.4f+/-%.4f | %7.2f | %7.2f | %-6s\n",
               res.conc, tipo, N_eventos, res.N_low, res.N_high,
               res.Q, res.errQ, res.Q, errQ_norm, Z_real, Z_norm, det);
        
        delete h;
        f->Close();
    }
    
    std::cout << std::string(140, '-') << std::endl;
    std::cout << "[INFO] Muestras procesadas: " << nMuestrasOK << "/" << MUESTRAS.size() << std::endl;
    std::cout << "[INFO] Z_real = Z con errores medidos | Z_norm = Z si tuvieramos " << (int)N_eventos_ref << " eventos" << std::endl;
    
    if (nMuestrasOK < 3) {
        std::cerr << "[ERROR] Muy pocas muestras para análisis" << std::endl;
        return;
    }
    
    // =========================================================================
    // PASO 3: Separar datos por tipo (fino vs grueso)
    // =========================================================================
    
    std::vector<double> C_fino, Q_fino, errQ_fino;
    std::vector<double> C_grueso, Q_grueso, errQ_grueso;
    std::vector<double> C_all, Q_all, errQ_all, zero_all;
    std::vector<double> Z_fino;
    
    for (const auto& r : resultados) {
        C_all.push_back(r.conc);
        Q_all.push_back(r.Q);
        errQ_all.push_back(r.errQ);
        zero_all.push_back(0);
        
        if (r.esFino) {
            C_fino.push_back(r.conc);
            Q_fino.push_back(r.Q);
            errQ_fino.push_back(r.errQ);
            Z_fino.push_back(r.Z_score);
        } else {
            C_grueso.push_back(r.conc);
            Q_grueso.push_back(r.Q);
            errQ_grueso.push_back(r.errQ);
        }
    }
    
    // =========================================================================
    // PASO 4: VALIDACIÓN DE LOD (usando barrido fino)
    // =========================================================================
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  VALIDACION DE LIMITE DE DETECCION (LOD)" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    std::cout << "\n[INFO] Analisis de Z-score (desviaciones del blanco):" << std::endl;
    std::cout << "       |Z| > 3  => Detectable (99.7% confianza)" << std::endl;
    std::cout << "       |Z| > 10 => Cuantificable (precision < 10%)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    double LOD_experimental = -1;
    double LOQ_experimental = -1;
    
    // Análisis de cada concentración del barrido fino
    std::cout << "\n  Barrido FINO - ¿Cuanta estadistica necesitamos?" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    printf("  %-8s | %-10s | %-12s | %-20s\n", "C(%)", "Z_actual", "Estado", "Factor para Z=3");
    std::cout << std::string(70, '-') << std::endl;
    
    for (size_t i = 0; i < resultados.size(); i++) {
        if (!resultados[i].esFino) continue;
        if (resultados[i].conc < 0.01) continue; // Saltar referencia
        
        double Z = resultados[i].Z_score;
        const char* estado;
        
        if (resultados[i].cuantificable) {
            estado = "CUANTIFICABLE";
            if (LOQ_experimental < 0) LOQ_experimental = resultados[i].conc;
        } else if (resultados[i].detectable) {
            estado = "DETECTABLE";
            if (LOD_experimental < 0) LOD_experimental = resultados[i].conc;
        } else {
            estado = "NO DETECTABLE";
        }
        
        // ¿Cuánta más estadística necesitamos para Z = 3?
        // Z ∝ sqrt(N), entonces N_needed/N_actual = (3/Z_actual)²
        double factor_needed = (std::abs(Z) > 0.1) ? std::pow(3.0 / std::abs(Z), 2) : 999;
        
        if (factor_needed < 1.0) {
            printf("  %-8.2f | %10.2f | %-12s | Ya detectado\n", 
                   resultados[i].conc, Z, estado);
        } else if (factor_needed < 100) {
            printf("  %-8.2f | %10.2f | %-12s | x%.1f mas eventos\n", 
                   resultados[i].conc, Z, estado, factor_needed);
        } else {
            printf("  %-8.2f | %10.2f | %-12s | >100x (muy dificil)\n", 
                   resultados[i].conc, Z, estado);
        }
    }
    std::cout << std::string(70, '-') << std::endl;
    
    // =========================================================================
    // PASO 5: Ajuste lineal para calibración (todos los datos)
    // =========================================================================
    
    TCanvas* cCalib = new TCanvas("cCalib", "Calibracion Completa", 1200, 800);
    cCalib->Divide(2, 1);
    
    // --- Panel 1: Calibración completa ---
    cCalib->cd(1);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.12);
    
    TGraphErrors* grAll = new TGraphErrors(C_all.size(), &C_all[0], &Q_all[0],
                                            &zero_all[0], &errQ_all[0]);
    grAll->SetTitle("Curva de Calibracion (todos los datos);C_{REE} (%);Q = N_{low}/N_{high}");
    grAll->SetMarkerStyle(21);
    grAll->SetMarkerSize(1.2);
    grAll->SetMarkerColor(kBlue+1);
    grAll->SetLineColor(kBlue+1);
    grAll->Draw("AP");
    
    // Ajuste lineal
    TF1* fLin = new TF1("fLin", "[0] + [1]*x", -0.2, 5.5);
    fLin->SetLineColor(kRed);
    fLin->SetLineWidth(2);
    grAll->Fit("fLin", "RQ");
    
    double p0 = fLin->GetParameter(0);  // Q0 (intercepto)
    double p1 = fLin->GetParameter(1);  // Sensibilidad k
    double ep0 = fLin->GetParError(0);
    double ep1 = fLin->GetParError(1);
    double chi2 = fLin->GetChisquare();
    int ndf = fLin->GetNDF();
    
    // Calcular LOD teórico usando error normalizado promedio
    // (todos los errores ahora son comparables)
    double sigma_Q_norm_prom = 0;
    int n_valid = 0;
    for (const auto& r : resultados) {
        // Recalcular error normalizado para esta muestra
        // El error normalizado ya está implícito en el Z-score
        if (r.errQ > 0 && r.errQ < 1) {
            sigma_Q_norm_prom += r.errQ;
            n_valid++;
        }
    }
    if (n_valid > 0) sigma_Q_norm_prom /= n_valid;
    
    // Para el LOD teórico, usar el error de la REFERENCIA (misma estadística)
    double precision_teorica = (std::abs(p1) > 1e-9) ? errQ0 / std::abs(p1) : 999;
    double LOD_teorico = 3.0 * precision_teorica;
    double LOQ_teorico = 10.0 * precision_teorica;
    
    // Caja de información
    TPaveText* pt1 = new TPaveText(0.48, 0.55, 0.88, 0.88, "NDC");
    pt1->SetFillColor(kWhite);
    pt1->SetBorderSize(1);
    pt1->SetTextAlign(12);
    pt1->SetTextFont(42);
    pt1->SetTextSize(0.032);
    pt1->AddText("Modelo: Q = Q_{0} + k #cdot C_{REE}");
    pt1->AddText(Form("Q_{0} = %.4f #pm %.4f", p0, ep0));
    pt1->AddText(Form("k = %.5f #pm %.5f [1/%%]", p1, ep1));
    pt1->AddText(Form("#chi^{2}/ndf = %.2f", (ndf > 0 ? chi2/ndf : 0)));
    pt1->AddText("");
    pt1->AddText(Form("Precision: #pm %.3f %%", precision_teorica));
    pt1->AddText(Form("LOD (3#sigma): %.3f %%", LOD_teorico));
    pt1->AddText(Form("LOQ (10#sigma): %.3f %%", LOQ_teorico));
    pt1->Draw();
    
    // --- Panel 2: Zoom en región LOD (barrido fino) ---
    cCalib->cd(2);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.12);
    
    std::vector<double> zero_fino(C_fino.size(), 0);
    TGraphErrors* grFino = new TGraphErrors(C_fino.size(), &C_fino[0], &Q_fino[0],
                                             &zero_fino[0], &errQ_fino[0]);
    grFino->SetTitle("Validacion LOD (barrido fino);C_{REE} (%);Q = N_{low}/N_{high}");
    grFino->SetMarkerStyle(20);
    grFino->SetMarkerSize(1.5);
    grFino->SetMarkerColor(kGreen+2);
    grFino->SetLineColor(kGreen+2);
    grFino->Draw("AP");
    
    // Línea del modelo
    TF1* fLinZoom = new TF1("fLinZoom", "[0] + [1]*x", -0.1, 1.2);
    fLinZoom->SetParameters(p0, p1);
    fLinZoom->SetLineColor(kRed);
    fLinZoom->SetLineStyle(2);
    fLinZoom->Draw("same");
    
    // Banda de 3-sigma (LOD)
    double y_3sigma_up = p0 + 3*errQ0;
    double y_3sigma_down = p0 - 3*errQ0;
    
    TLine* l3up = new TLine(-0.1, y_3sigma_up, 1.2, y_3sigma_up);
    l3up->SetLineColor(kOrange+1);
    l3up->SetLineStyle(2);
    l3up->SetLineWidth(2);
    l3up->Draw();
    
    TLine* l3down = new TLine(-0.1, y_3sigma_down, 1.2, y_3sigma_down);
    l3down->SetLineColor(kOrange+1);
    l3down->SetLineStyle(2);
    l3down->SetLineWidth(2);
    l3down->Draw();
    
    // Línea horizontal Q0
    TLine* lQ0 = new TLine(-0.1, p0, 1.2, p0);
    lQ0->SetLineColor(kBlack);
    lQ0->SetLineStyle(1);
    lQ0->Draw();
    
    TLegend* leg2 = new TLegend(0.15, 0.70, 0.55, 0.88);
    leg2->AddEntry(grFino, "Datos barrido fino", "ep");
    leg2->AddEntry(fLinZoom, "Modelo calibracion", "l");
    leg2->AddEntry(l3up, "Banda 3#sigma (LOD)", "l");
    leg2->Draw();
    
    // Etiqueta LOD experimental
    TPaveText* pt2 = new TPaveText(0.55, 0.15, 0.88, 0.35, "NDC");
    pt2->SetFillColor(kWhite);
    pt2->SetBorderSize(1);
    pt2->SetTextSize(0.035);
    if (LOD_experimental > 0) {
        pt2->AddText(Form("LOD exp: %.2f %%", LOD_experimental));
    } else {
        pt2->AddText("LOD exp: < 0.20 %");
    }
    if (LOQ_experimental > 0) {
        pt2->AddText(Form("LOQ exp: %.2f %%", LOQ_experimental));
    }
    pt2->Draw();
    
    cCalib->SaveAs("Eu152_v4_Calibracion.png");
    
    // =========================================================================
    // PASO 6: Gráfico de Z-score vs Concentración
    // =========================================================================
    
    TCanvas* cZ = new TCanvas("cZ", "Z-score", 900, 600);
    cZ->SetGrid();
    cZ->SetLeftMargin(0.12);
    
    std::vector<double> C_Zscore, Z_valores;
    for (const auto& r : resultados) {
        C_Zscore.push_back(r.conc);
        Z_valores.push_back(r.Z_score);
    }
    
    TGraph* grZ = new TGraph(C_Zscore.size(), &C_Zscore[0], &Z_valores[0]);
    grZ->SetTitle("Z-score vs Concentracion;C_{REE} (%);Z-score = (Q - Q_{0})/#sigma");
    grZ->SetMarkerStyle(21);
    grZ->SetMarkerSize(1.3);
    grZ->SetMarkerColor(kBlue+1);
    grZ->SetLineColor(kBlue+1);
    grZ->SetLineWidth(2);
    grZ->Draw("ALP");
    
    // Líneas de umbral
    TLine* lZ3 = new TLine(-0.2, 3, 5.5, 3);
    lZ3->SetLineColor(kOrange+1);
    lZ3->SetLineStyle(2);
    lZ3->SetLineWidth(2);
    lZ3->Draw();
    
    TLine* lZ10 = new TLine(-0.2, 10, 5.5, 10);
    lZ10->SetLineColor(kRed);
    lZ10->SetLineStyle(2);
    lZ10->SetLineWidth(2);
    lZ10->Draw();
    
    TLine* lZ0 = new TLine(-0.2, 0, 5.5, 0);
    lZ0->SetLineColor(kBlack);
    lZ0->SetLineStyle(1);
    lZ0->Draw();
    
    TLegend* legZ = new TLegend(0.15, 0.65, 0.45, 0.88);
    legZ->AddEntry(grZ, "Z-score", "lp");
    legZ->AddEntry(lZ3, "LOD (Z = 3)", "l");
    legZ->AddEntry(lZ10, "LOQ (Z = 10)", "l");
    legZ->Draw();
    
    TPaveText* ptZ = new TPaveText(0.55, 0.15, 0.88, 0.40, "NDC");
    ptZ->SetFillColor(kWhite);
    ptZ->SetBorderSize(1);
    ptZ->AddText("Criterios:");
    ptZ->AddText("|Z| > 3 : Detectable");
    ptZ->AddText("|Z| > 10 : Cuantificable");
    ptZ->Draw();
    
    cZ->SaveAs("Eu152_v4_Zscore.png");
    
    // =========================================================================
    // PASO 7: Espectros comparativos (blanco vs concentraciones)
    // =========================================================================
    
    TCanvas* cSpec = new TCanvas("cSpec", "Espectros Comparativos", 1400, 900);
    cSpec->Divide(2, 2);
    
    // Cargar algunos espectros representativos
    int indices[] = {0, 2, 5, 9};  // 0%, 0.4%, 1.0%, 5.0%
    int colors[] = {kBlack, kBlue, kGreen+2, kRed};
    
    TH1D* hSpec[4] = {nullptr, nullptr, nullptr, nullptr};
    
    for (int j = 0; j < 4; j++) {
        int idx = indices[j];
        if (idx >= (int)MUESTRAS.size()) continue;
        
        TString filename = Form("%s%s", directorio, MUESTRAS[idx].nombre.Data());
        TFile* f = TFile::Open(filename, "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "[WARN] No se pudo abrir para espectro: " << filename << std::endl;
            continue;
        }
        
        TTree* t = (TTree*)f->Get("Scoring");
        if (!t) { f->Close(); continue; }
        
        // Crear histograma DESVINCULADO del archivo (SetDirectory(0))
        TString hname = Form("hSpec_%d", j);
        hSpec[j] = new TH1D(hname, "", 1600, 0, 1600);
        hSpec[j]->SetDirectory(0);  // IMPORTANTE: Desvincula del archivo
        
        // Llenar manualmente para evitar problemas
        Double_t energy;
        t->SetBranchAddress("Energy", &energy);
        Long64_t nentries = t->GetEntries();
        for (Long64_t n = 0; n < nentries; n++) {
            t->GetEntry(n);
            hSpec[j]->Fill(energy * 1000.0);  // MeV -> keV
        }
        
        f->Close();
        delete f;
        
        // Ahora dibujar (el histograma sigue en memoria)
        cSpec->cd(j + 1);
        gPad->SetLogy();
        gPad->SetGrid();
        
        hSpec[j]->SetLineColor(colors[j]);
        hSpec[j]->SetLineWidth(2);
        hSpec[j]->SetTitle(Form("Espectro %.2f%% REE;Energia (keV);Cuentas", MUESTRAS[idx].concentracion));
        hSpec[j]->GetXaxis()->SetRangeUser(50, 1500);
        hSpec[j]->Draw();
        
        // Marcar ROIs
        double ymax = hSpec[j]->GetMaximum();
        
        TLine* l1 = new TLine(E_LOW - ventana_low, 1, E_LOW - ventana_low, ymax/5);
        TLine* l2 = new TLine(E_LOW + ventana_low, 1, E_LOW + ventana_low, ymax/5);
        l1->SetLineColor(kRed); l1->SetLineStyle(2); l1->Draw();
        l2->SetLineColor(kRed); l2->SetLineStyle(2); l2->Draw();
        
        TLine* l3 = new TLine(E_HIGH - ventana_high, 1, E_HIGH - ventana_high, ymax/20);
        TLine* l4 = new TLine(E_HIGH + ventana_high, 1, E_HIGH + ventana_high, ymax/20);
        l3->SetLineColor(kGreen+2); l3->SetLineStyle(2); l3->Draw();
        l4->SetLineColor(kGreen+2); l4->SetLineStyle(2); l4->Draw();
        
        // Etiqueta con información
        TLatex* tex = new TLatex();
        tex->SetNDC();
        tex->SetTextSize(0.04);
        tex->DrawLatex(0.55, 0.80, Form("C = %.2f%%", MUESTRAS[idx].concentracion));
        tex->DrawLatex(0.55, 0.75, Form("Eventos: %.0f", (double)nentries));
    }
    
    cSpec->Update();
    cSpec->SaveAs("Eu152_v4_Espectros.png");
    
    // =========================================================================
    // PASO 8: Resumen de Detectabilidad
    // =========================================================================
    
    TCanvas* cDetect = new TCanvas("cDetect", "Mapa de Detectabilidad", 900, 600);
    cDetect->SetGrid();
    cDetect->SetLeftMargin(0.12);
    
    // Crear gráfico con colores según detectabilidad
    std::vector<double> C_det, Q_det, C_nodet, Q_nodet, C_cuant, Q_cuant;
    
    for (const auto& r : resultados) {
        if (r.cuantificable) {
            C_cuant.push_back(r.conc);
            Q_cuant.push_back(r.Q);
        } else if (r.detectable) {
            C_det.push_back(r.conc);
            Q_det.push_back(r.Q);
        } else {
            C_nodet.push_back(r.conc);
            Q_nodet.push_back(r.Q);
        }
    }
    
    TMultiGraph* mg = new TMultiGraph();
    mg->SetTitle("Mapa de Detectabilidad;C_{REE} (%);Q = N_{low}/N_{high}");
    
    if (C_nodet.size() > 0) {
        TGraph* grNodet = new TGraph(C_nodet.size(), &C_nodet[0], &Q_nodet[0]);
        grNodet->SetMarkerStyle(21);
        grNodet->SetMarkerSize(1.5);
        grNodet->SetMarkerColor(kGray+1);
        mg->Add(grNodet, "P");
    }
    
    if (C_det.size() > 0) {
        TGraph* grDet = new TGraph(C_det.size(), &C_det[0], &Q_det[0]);
        grDet->SetMarkerStyle(21);
        grDet->SetMarkerSize(1.5);
        grDet->SetMarkerColor(kOrange+1);
        mg->Add(grDet, "P");
    }
    
    if (C_cuant.size() > 0) {
        TGraph* grCuant = new TGraph(C_cuant.size(), &C_cuant[0], &Q_cuant[0]);
        grCuant->SetMarkerStyle(21);
        grCuant->SetMarkerSize(1.5);
        grCuant->SetMarkerColor(kGreen+2);
        mg->Add(grCuant, "P");
    }
    
    mg->Draw("A");
    fLin->Draw("same");
    
    TLegend* legD = new TLegend(0.15, 0.65, 0.50, 0.88);
    legD->AddEntry((TObject*)nullptr, Form("NO detectable (Z<3): %zu", C_nodet.size()), "");
    legD->AddEntry((TObject*)nullptr, Form("Detectable (3<Z<10): %zu", C_det.size()), "");
    legD->AddEntry((TObject*)nullptr, Form("Cuantificable (Z>10): %zu", C_cuant.size()), "");
    legD->Draw();
    
    cDetect->SaveAs("Eu152_v4_Detectabilidad.png");
    
    // =========================================================================
    // RESUMEN FINAL EN CONSOLA
    // =========================================================================
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  RESUMEN FINAL - ANALISIS Eu-152 v4" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    printf("\n>> CONFIGURACION:\n");
    printf("   Linea baja:  %.1f keV (ventana +/-%.0f keV)\n", E_LOW, ventana_low);
    printf("   Linea alta:  %.1f keV (ventana +/-%.0f keV)\n", E_HIGH, ventana_high);
    printf("   Muestras procesadas: %d/%zu\n", nMuestrasOK, MUESTRAS.size());
    
    printf("\n>> MODELO DE CALIBRACION:\n");
    printf("   Q = Q0 + k * C_REE\n");
    printf("   Q0 = %.5f +/- %.5f\n", p0, ep0);
    printf("   k  = %.6f +/- %.6f [1/%%]\n", p1, ep1);
    printf("   chi2/ndf = %.3f\n", (ndf > 0 ? chi2/ndf : 0));
    
    printf("\n>> LIMITES DE DETECCION:\n");
    printf("   --- Teoricos (basado en error de referencia) ---\n");
    printf("   sigma(Q0) = %.5f\n", errQ0);
    printf("   Precision: +/- %.3f %% REE\n", precision_teorica);
    printf("   LOD (3-sigma): %.3f %% REE\n", LOD_teorico);
    printf("   LOQ (10-sigma): %.3f %% REE\n", LOQ_teorico);
    printf("\n   --- Experimentales (barrido fino) ---\n");
    printf("   Estadistica referencia: %.0f eventos\n", N_eventos_ref);
    if (LOD_experimental > 0) {
        printf("   LOD observado: %.2f %% REE\n", LOD_experimental);
    } else {
        printf("   LOD observado: < 0.20 %% REE (primera conc. medida)\n");
    }
    if (LOQ_experimental > 0) {
        printf("   LOQ observado: %.2f %% REE\n", LOQ_experimental);
    }
    
    printf("\n>> ESTADISTICAS DE DETECTABILIDAD:\n");
    printf("   No detectables (Z<3):    %zu muestras\n", C_nodet.size());
    printf("   Detectables (3<Z<10):    %zu muestras\n", C_det.size());
    printf("   Cuantificables (Z>10):   %zu muestras\n", C_cuant.size());
    
    printf("\n>> PARA MUESTRA DESCONOCIDA:\n");
    printf("   C_REE = (Q - %.5f) / (%.6f)\n", p0, p1);
    printf("   Error: delta_C = delta_Q / |k| = delta_Q / %.6f\n", std::abs(p1));
    
    printf("\n>> ESCALAMIENTO CON ESTADISTICA:\n");
    printf("   El Z-score escala como: Z ~ sqrt(N_eventos)\n");
    printf("   Para mejorar LOD por factor 2: necesitas 4x mas eventos\n");
    printf("   Para mejorar LOD por factor 3: necesitas 9x mas eventos\n");
    printf("   Eventos actuales (ref): %.0f\n", N_eventos_ref);
    
    printf("\n>> ARCHIVOS GENERADOS:\n");
    printf("   - Eu152_v4_Calibracion.png\n");
    printf("   - Eu152_v4_Zscore.png\n");
    printf("   - Eu152_v4_Espectros.png\n");
    printf("   - Eu152_v4_Detectabilidad.png\n");
    
    std::cout << std::string(80, '=') << std::endl;
    
    // =========================================================================
    // Guardar resultados a archivo CSV
    // =========================================================================
    
    std::ofstream csv("Eu152_v4_resultados.csv");
    csv << "Concentracion,Tipo,N_low,errN_low,N_high,errN_high,Q,errQ,Z_score,Detectable,Cuantificable" << std::endl;
    for (const auto& r : resultados) {
        csv << r.conc << "," 
            << (r.esFino ? "FINO" : "GRUESO") << ","
            << r.N_low << "," << r.errN_low << ","
            << r.N_high << "," << r.errN_high << ","
            << r.Q << "," << r.errQ << ","
            << r.Z_score << ","
            << (r.detectable ? "SI" : "NO") << ","
            << (r.cuantificable ? "SI" : "NO") << std::endl;
    }
    csv.close();
    std::cout << "\n[INFO] Resultados guardados en: Eu152_v4_resultados.csv" << std::endl;
}
