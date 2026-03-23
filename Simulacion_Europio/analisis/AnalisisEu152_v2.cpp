/**
 * @file AnalisisEu152.cpp
 * @brief Análisis Dual-Energy con Eu-152 para cuantificación de REE
 * 
 * Archivos: Eu152_REE_0pXX.root (TTree: Scoring, Branch: Energy en MeV)
 * 
 * Método: Integración directa en ventanas fijas (robusto)
 * 
 * Uso: root -l 'AnalisisEu152.cpp("./")'
 *      root -l 'AnalisisEu152.cpp("./", true)'  // para usar 1408 keV
 */

#include <iostream>
#include <vector>
#include <cmath>

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

// Muestras
const int N_MUESTRAS = 6;
const double CONC[N_MUESTRAS] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};

// ============================================================================
// ESTRUCTURA PARA RESULTADOS
// ============================================================================

struct Pico {
    double cuentas_brutas;
    double cuentas_netas;
    double error;
    double fondo;
    bool ok;
    
    Pico() : cuentas_brutas(0), cuentas_netas(0), error(0), fondo(0), ok(false) {}
};

// ============================================================================
// FUNCIÓN: Integrar fotopico con sustracción de fondo
// ============================================================================

Pico IntegrarFotopico(TH1D* h, double E_centro, double semi_ancho) {
    Pico p;
    if (!h) return p;
    
    // =====================================================
    // ROI del pico: [E_centro - semi_ancho, E_centro + semi_ancho]
    // =====================================================
    int bin_pico_min = h->FindBin(E_centro - semi_ancho);
    int bin_pico_max = h->FindBin(E_centro + semi_ancho);
    
    p.cuentas_brutas = h->Integral(bin_pico_min, bin_pico_max);
    
    // =====================================================
    // Regiones de fondo: bandas laterales
    // Izquierda: [E - 2*ancho, E - ancho]
    // Derecha:   [E + ancho, E + 2*ancho]
    // =====================================================
    int bin_bg_L1 = h->FindBin(E_centro - 2*semi_ancho);
    int bin_bg_L2 = h->FindBin(E_centro - semi_ancho) - 1;
    int bin_bg_R1 = h->FindBin(E_centro + semi_ancho) + 1;
    int bin_bg_R2 = h->FindBin(E_centro + 2*semi_ancho);
    
    double bg_izq = h->Integral(bin_bg_L1, bin_bg_L2);
    double bg_der = h->Integral(bin_bg_R1, bin_bg_R2);
    
    // Número de bines
    int n_pico = bin_pico_max - bin_pico_min + 1;
    int n_bg_izq = bin_bg_L2 - bin_bg_L1 + 1;
    int n_bg_der = bin_bg_R2 - bin_bg_R1 + 1;
    int n_bg_total = n_bg_izq + n_bg_der;
    
    // Fondo escalado al tamaño del ROI del pico
    double bg_total = bg_izq + bg_der;
    p.fondo = bg_total * (double)n_pico / (double)n_bg_total;
    
    // Cuentas netas
    p.cuentas_netas = p.cuentas_brutas - p.fondo;
    
    // Error estadístico: sqrt(N_brutas + N_fondo_escalado)
    // (contribución del pico + contribución del fondo)
    p.error = std::sqrt(p.cuentas_brutas + p.fondo);
    
    // Validar: SNR > 3
    p.ok = (p.cuentas_netas > 0 && p.cuentas_netas > 3 * p.error);
    
    return p;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void AnalisisEu152_v2(const char* directorio = "./", bool usar_1408 = false) {
    
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(1111);
    
    // =========================================================================
    // Seleccionar líneas para análisis dual-energy
    // =========================================================================
    double E_LOW = E_122;      // Siempre 122 keV (fotoeléctrico)
    double E_HIGH = usar_1408 ? E_1408 : E_779;
    
    // Ventanas de integración (semi-ancho en keV)
    // Ajustadas según resolución típica del detector
    double ventana_low = 12.0;   // ±12 keV para 122 keV
    double ventana_high = usar_1408 ? 25.0 : 20.0;  // ±20-25 keV para alta E
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "  ANALISIS Eu-152 DUAL-ENERGY PARA CUANTIFICACION DE REE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "  Linea BAJA:  " << E_LOW << " keV (fotoelectrico, sensible a Z)" << std::endl;
    std::cout << "  Linea ALTA:  " << E_HIGH << " keV (Compton, sensible a densidad)" << std::endl;
    std::cout << "  Ventanas:    +/-" << ventana_low << " keV, +/-" << ventana_high << " keV" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // =========================================================================
    // PASO 1: Cargar REFERENCIA (0% REE)
    // =========================================================================
    
    TString filename_ref = Form("%sEu152_REE_0p00.root", directorio);
    TFile* f_ref = TFile::Open(filename_ref);
    if (!f_ref || f_ref->IsZombie()) {
        std::cerr << "[ERROR] No se puede abrir: " << filename_ref << std::endl;
        return;
    }
    
    TTree* t_ref = (TTree*)f_ref->Get("Scoring");
    if (!t_ref) {
        std::cerr << "[ERROR] TTree 'Scoring' no encontrado" << std::endl;
        return;
    }
    
    // Histograma de referencia
    TH1D* h_ref = new TH1D("h_ref", "Referencia 0% REE", 1600, 0, 1600);
    t_ref->Draw("Energy*1000>>h_ref", "", "goff");
    
    std::cout << "\n[INFO] Referencia cargada: " << (int)h_ref->GetEntries() << " eventos\n";
    
    // Integrar picos de referencia
    Pico ref_low = IntegrarFotopico(h_ref, E_LOW, ventana_low);
    Pico ref_high = IntegrarFotopico(h_ref, E_HIGH, ventana_high);
    
    std::cout << "\n--- REFERENCIA I0 (0% REE) ---" << std::endl;
    printf("  %.0f keV: Netas = %.0f +/- %.0f (brutas=%.0f, fondo=%.0f)\n",
           E_LOW, ref_low.cuentas_netas, ref_low.error, 
           ref_low.cuentas_brutas, ref_low.fondo);
    printf("  %.0f keV: Netas = %.0f +/- %.0f (brutas=%.0f, fondo=%.0f)\n",
           E_HIGH, ref_high.cuentas_netas, ref_high.error,
           ref_high.cuentas_brutas, ref_high.fondo);
    
    if (ref_low.cuentas_netas <= 0 || ref_high.cuentas_netas <= 0) {
        std::cerr << "\n[ERROR] Cuentas netas <= 0 en referencia. Verifica las ventanas." << std::endl;
        return;
    }
    
    // =========================================================================
    // PASO 2: Procesar todas las muestras
    // =========================================================================
    
    std::vector<double> C_vec, R_vec, errR_vec, zero_vec;
    std::vector<double> T_low_vec, T_high_vec;
    std::vector<double> L_low_vec, L_high_vec;
    
    // Canvas para espectros
    TCanvas* cSpec = new TCanvas("cSpec", "Espectros Eu-152", 1500, 1000);
    cSpec->Divide(3, 2);
    
    std::cout << "\n" << std::string(100, '-') << std::endl;
    printf("%-5s | %-10s | %-10s | %-8s | %-8s | %-10s | %-10s | %-15s\n",
           "C(%)", "N_low", "N_high", "T_low", "T_high", "L_low", "L_high", "R +/- err");
    std::cout << std::string(100, '-') << std::endl;
    
    for (int i = 0; i < N_MUESTRAS; i++) {
        
        // Abrir archivo
        TString filename = Form("%sEu152_REE_0p%02d.root", directorio, (int)CONC[i]);
        TFile* f = TFile::Open(filename);
        if (!f || f->IsZombie()) {
            std::cerr << "[WARN] No se puede abrir: " << filename << std::endl;
            continue;
        }
        
        TTree* t = (TTree*)f->Get("Scoring");
        if (!t) continue;
        
        // Crear histograma
        TString hname = Form("h_%d", i);
        TH1D* h = new TH1D(hname, Form("%.0f%% REE", CONC[i]), 1600, 0, 1600);
        t->Draw(Form("Energy*1000>>%s", hname.Data()), "", "goff");
        
        // Integrar picos
        Pico p_low = IntegrarFotopico(h, E_LOW, ventana_low);
        Pico p_high = IntegrarFotopico(h, E_HIGH, ventana_high);
        
        // Dibujar espectro
        cSpec->cd(i + 1);
        gPad->SetLogy();
        gPad->SetGridx();
        gPad->SetGridy();
        h->SetLineColor(kBlue);
        h->GetXaxis()->SetTitle("Energia (keV)");
        h->GetYaxis()->SetTitle("Cuentas");
        h->Draw();
        
        // Marcar ROIs
        TLine* l1 = new TLine(E_LOW - ventana_low, 1, E_LOW - ventana_low, h->GetMaximum()/5);
        TLine* l2 = new TLine(E_LOW + ventana_low, 1, E_LOW + ventana_low, h->GetMaximum()/5);
        l1->SetLineColor(kRed); l1->SetLineStyle(2); l1->Draw();
        l2->SetLineColor(kRed); l2->SetLineStyle(2); l2->Draw();
        
        TLine* l3 = new TLine(E_HIGH - ventana_high, 1, E_HIGH - ventana_high, h->GetMaximum()/20);
        TLine* l4 = new TLine(E_HIGH + ventana_high, 1, E_HIGH + ventana_high, h->GetMaximum()/20);
        l3->SetLineColor(kGreen+2); l3->SetLineStyle(2); l3->Draw();
        l4->SetLineColor(kGreen+2); l4->SetLineStyle(2); l4->Draw();
        
        TLatex* tex = new TLatex(0.65, 0.85, Form("%.0f%% REE", CONC[i]));
        tex->SetNDC(); tex->SetTextSize(0.05); tex->Draw();
        
        // =====================================================
        // Calcular transmisiones T = I / I0
        // =====================================================
        double T_low = p_low.cuentas_netas / ref_low.cuentas_netas;
        double T_high = p_high.cuentas_netas / ref_high.cuentas_netas;
        
        // =====================================================
        // Calcular atenuaciones L = -ln(T) = ln(I0/I)
        // =====================================================
        double L_low = 0, L_high = 0;
        if (T_low > 0) L_low = -std::log(T_low);
        if (T_high > 0) L_high = -std::log(T_high);
        
        // =====================================================
        // Calcular RATIO R = L_low / L_high
        // =====================================================
        double R = 0, errR = 0;
        
        if (L_high > 0.001) {  // Evitar división por ~0
            R = L_low / L_high;
            
            // Propagación de error completa
            double I = p_low.cuentas_netas;
            double I0 = ref_low.cuentas_netas;
            double J = p_high.cuentas_netas;
            double J0 = ref_high.cuentas_netas;
            
            double dI = p_low.error;
            double dI0 = ref_low.error;
            double dJ = p_high.error;
            double dJ0 = ref_high.error;
            
            // R = ln(I0/I) / ln(J0/J)
            // dR/dI = -1/(I * L_high)
            // dR/dI0 = 1/(I0 * L_high)
            // dR/dJ = R/(J * L_high)
            // dR/dJ0 = -R/(J0 * L_high)
            
            double dR_dI = -1.0 / (I * L_high);
            double dR_dI0 = 1.0 / (I0 * L_high);
            double dR_dJ = R / (J * L_high);
            double dR_dJ0 = -R / (J0 * L_high);
            
            errR = std::sqrt(
                std::pow(dR_dI * dI, 2) +
                std::pow(dR_dI0 * dI0, 2) +
                std::pow(dR_dJ * dJ, 2) +
                std::pow(dR_dJ0 * dJ0, 2)
            );
        }
        
        // Guardar datos
        C_vec.push_back(CONC[i]);
        R_vec.push_back(R);
        errR_vec.push_back(errR);
        zero_vec.push_back(0);
        T_low_vec.push_back(T_low);
        T_high_vec.push_back(T_high);
        L_low_vec.push_back(L_low);
        L_high_vec.push_back(L_high);
        
        printf("%-5.1f | %-10.0f | %-10.0f | %-8.4f | %-8.4f | %-10.4f | %-10.4f | %.4f +/- %.4f\n",
               CONC[i], p_low.cuentas_netas, p_high.cuentas_netas,
               T_low, T_high, L_low, L_high, R, errR);
    }
    
    std::cout << std::string(100, '-') << std::endl;
    cSpec->SaveAs("Eu152_Espectros.png");
    
    // =========================================================================
    // PASO 3: Gráfico de Transmisiones
    // =========================================================================
    
    TCanvas* cTrans = new TCanvas("cTrans", "Transmisiones", 900, 600);
    cTrans->SetGrid();
    
    TGraph* grT_low = new TGraph(C_vec.size(), &C_vec[0], &T_low_vec[0]);
    grT_low->SetTitle(Form("Transmision vs C_{REE};Concentracion REE (%%);T = I/I_{0}"));
    grT_low->SetMarkerStyle(20);
    grT_low->SetMarkerSize(1.3);
    grT_low->SetMarkerColor(kRed);
    grT_low->SetLineColor(kRed);
    grT_low->SetLineWidth(2);
    
    TGraph* grT_high = new TGraph(C_vec.size(), &C_vec[0], &T_high_vec[0]);
    grT_high->SetMarkerStyle(21);
    grT_high->SetMarkerSize(1.3);
    grT_high->SetMarkerColor(kBlue);
    grT_high->SetLineColor(kBlue);
    grT_high->SetLineWidth(2);
    
    grT_low->Draw("ALP");
    grT_low->GetYaxis()->SetRangeUser(0, 1.1);
    grT_high->Draw("LP same");
    
    TLegend* legT = new TLegend(0.55, 0.7, 0.88, 0.88);
    legT->AddEntry(grT_low, Form("T @ %.0f keV (PE)", E_LOW), "lp");
    legT->AddEntry(grT_high, Form("T @ %.0f keV (Compton)", E_HIGH), "lp");
    legT->Draw();
    
    cTrans->SaveAs("Eu152_Transmisiones.png");
    
    // =========================================================================
    // PASO 4: Calcular MÚLTIPLES ÍNDICES para comparación
    // =========================================================================
    
    // Índice 1: R = L_low / L_high (ratio de logaritmos - original)
    // Ya calculado en R_vec
    
    // Índice 2: Ratio de cuentas Q = N_low / N_high (más robusto)
    std::vector<double> Q_vec, errQ_vec;
    
    // Índice 3: Diferencia de atenuaciones Delta = L_low - L_high
    std::vector<double> Delta_vec, errDelta_vec;
    
    // Índice 4: Solo L_low (atenuación baja energía)
    std::vector<double> errL_low_vec;
    
    double N0_low = ref_low.cuentas_netas;
    double N0_high = ref_high.cuentas_netas;
    
    for (size_t i = 0; i < C_vec.size(); i++) {
        // Recalcular cuentas
        TString filename = Form("%sEu152_REE_0p%02d.root", directorio, (int)C_vec[i]);
        TFile* f = TFile::Open(filename, "READ");
        TTree* t = (TTree*)f->Get("Scoring");
        TString hname = Form("hQ_%zu", i);
        TH1D* h = new TH1D(hname, "", 1600, 0, 1600);
        t->Draw(Form("Energy*1000>>%s", hname.Data()), "", "goff");
        
        Pico p_low = IntegrarFotopico(h, E_LOW, ventana_low);
        Pico p_high = IntegrarFotopico(h, E_HIGH, ventana_high);
        
        // Q = N_low / N_high
        double Q = p_low.cuentas_netas / p_high.cuentas_netas;
        double errQ = Q * std::sqrt(std::pow(p_low.error/p_low.cuentas_netas, 2) +
                                    std::pow(p_high.error/p_high.cuentas_netas, 2));
        Q_vec.push_back(Q);
        errQ_vec.push_back(errQ);
        
        // Delta = L_low - L_high
        double Delta = L_low_vec[i] - L_high_vec[i];
        double errDelta = std::sqrt(std::pow(p_low.error/p_low.cuentas_netas, 2) +
                                    std::pow(p_high.error/p_high.cuentas_netas, 2));
        Delta_vec.push_back(Delta);
        errDelta_vec.push_back(errDelta);
        
        // Error en L_low
        double errL = p_low.error / p_low.cuentas_netas;
        errL_low_vec.push_back(errL);
        
        delete h;
        f->Close();
    }
    
    // =========================================================================
    // PASO 5: Comparar los 4 índices
    // =========================================================================
    
    TCanvas* cComp = new TCanvas("cComp", "Comparacion de Indices", 1400, 1000);
    cComp->Divide(2, 2);
    
    double sens[4], err_sens[4], chi2_arr[4], precision_arr[4];
    const char* nombres[4] = {"R = L_low/L_high", "Q = N_low/N_high", 
                               "Delta = L_low - L_high", "L_low solo"};
    
    // --- Índice 1: R ---
    cComp->cd(1);
    gPad->SetGrid();
    TGraphErrors* gr1 = new TGraphErrors(C_vec.size(), &C_vec[0], &R_vec[0], &zero_vec[0], &errR_vec[0]);
    gr1->SetTitle("R = ln(T_{low})/ln(T_{high});C_{REE} (%);R");
    gr1->SetMarkerStyle(21); gr1->SetMarkerColor(kBlue); gr1->SetLineColor(kBlue);
    gr1->Draw("AP");
    TF1* f1 = new TF1("f1", "[0]+[1]*x", 0.5, 5.5);
    gr1->Fit("f1", "RQ");
    sens[0] = f1->GetParameter(1); err_sens[0] = f1->GetParError(1);
    chi2_arr[0] = (f1->GetNDF()>0) ? f1->GetChisquare()/f1->GetNDF() : 0;
    
    // --- Índice 2: Q ---
    cComp->cd(2);
    gPad->SetGrid();
    TGraphErrors* gr2 = new TGraphErrors(C_vec.size(), &C_vec[0], &Q_vec[0], &zero_vec[0], &errQ_vec[0]);
    gr2->SetTitle("Q = N_{low}/N_{high};C_{REE} (%);Q");
    gr2->SetMarkerStyle(21); gr2->SetMarkerColor(kRed); gr2->SetLineColor(kRed);
    gr2->Draw("AP");
    TF1* f2 = new TF1("f2", "[0]+[1]*x", -0.5, 5.5);
    gr2->Fit("f2", "RQ");
    sens[1] = f2->GetParameter(1); err_sens[1] = f2->GetParError(1);
    chi2_arr[1] = (f2->GetNDF()>0) ? f2->GetChisquare()/f2->GetNDF() : 0;
    
    // --- Índice 3: Delta ---
    cComp->cd(3);
    gPad->SetGrid();
    TGraphErrors* gr3 = new TGraphErrors(C_vec.size(), &C_vec[0], &Delta_vec[0], &zero_vec[0], &errDelta_vec[0]);
    gr3->SetTitle("#Delta = L_{low} - L_{high};C_{REE} (%);#Delta");
    gr3->SetMarkerStyle(21); gr3->SetMarkerColor(kGreen+2); gr3->SetLineColor(kGreen+2);
    gr3->Draw("AP");
    TF1* f3 = new TF1("f3", "[0]+[1]*x", -0.5, 5.5);
    gr3->Fit("f3", "RQ");
    sens[2] = f3->GetParameter(1); err_sens[2] = f3->GetParError(1);
    chi2_arr[2] = (f3->GetNDF()>0) ? f3->GetChisquare()/f3->GetNDF() : 0;
    
    // --- Índice 4: L_low ---
    cComp->cd(4);
    gPad->SetGrid();
    TGraphErrors* gr4 = new TGraphErrors(C_vec.size(), &C_vec[0], &L_low_vec[0], &zero_vec[0], &errL_low_vec[0]);
    gr4->SetTitle("L_{low} = -ln(T_{122});C_{REE} (%);L_{low}");
    gr4->SetMarkerStyle(21); gr4->SetMarkerColor(kMagenta); gr4->SetLineColor(kMagenta);
    gr4->Draw("AP");
    TF1* f4 = new TF1("f4", "[0]+[1]*x", -0.5, 5.5);
    gr4->Fit("f4", "RQ");
    sens[3] = f4->GetParameter(1); err_sens[3] = f4->GetParError(1);
    chi2_arr[3] = (f4->GetNDF()>0) ? f4->GetChisquare()/f4->GetNDF() : 0;
    
    cComp->SaveAs("Eu152_Comparacion_Indices.png");
    
    // =========================================================================
    // PASO 6: Tabla comparativa
    // =========================================================================
    
    std::cout << "\n" << std::string(85, '=') << std::endl;
    std::cout << "  COMPARACION DE INDICES" << std::endl;
    std::cout << std::string(85, '=') << std::endl;
    printf("\n%-28s | %-12s | %-12s | %-10s | %-12s\n",
           "Indice", "Sensib (k)", "Error k", "chi2/ndf", "Precision");
    std::cout << std::string(85, '-') << std::endl;
    
    std::vector<std::vector<double>*> err_vecs = {&errR_vec, &errQ_vec, &errDelta_vec, &errL_low_vec};
    
    for (int idx = 0; idx < 4; idx++) {
        double sigma_medio = 0;
        int n = 0;
        for (size_t k = 1; k < err_vecs[idx]->size(); k++) {
            double err = (*err_vecs[idx])[k];
            if (err > 0 && err < 1e6) { sigma_medio += err; n++; }
        }
        if (n > 0) sigma_medio /= n;
        precision_arr[idx] = (std::abs(sens[idx]) > 1e-9) ? sigma_medio / std::abs(sens[idx]) : 999;
        
        printf("%-28s | %-12.4f | %-12.4f | %-10.3f | %-10.2f %%\n",
               nombres[idx], sens[idx], err_sens[idx], chi2_arr[idx], precision_arr[idx]);
    }
    std::cout << std::string(85, '-') << std::endl;
    
    int mejor = 0;
    for (int idx = 1; idx < 4; idx++) {
        if (precision_arr[idx] < precision_arr[mejor]) mejor = idx;
    }
    std::cout << "\n>>> MEJOR INDICE: " << nombres[mejor] 
              << " (precision = +/- " << precision_arr[mejor] << " %)" << std::endl;
    
    // =========================================================================
    // PASO 7: Curva con el mejor índice (Q - ratio de cuentas)
    // =========================================================================
    
    TCanvas* cCalib = new TCanvas("cCalib", "Calibracion", 900, 700);
    cCalib->SetGrid();
    cCalib->SetLeftMargin(0.12);
    
    TGraphErrors* gr = new TGraphErrors(C_vec.size(),
                                         &C_vec[0], &Q_vec[0],
                                         &zero_vec[0], &errQ_vec[0]);
    
    gr->SetTitle(Form("Calibracion Eu-152 - Indice Q;Concentracion REE (%% peso);Q = N_{%.0f keV}/N_{%.0f keV}",
                      E_LOW, E_HIGH));
    gr->SetMarkerStyle(21);
    gr->SetMarkerSize(1.5);
    gr->SetMarkerColor(kBlue+1);
    gr->SetLineColor(kBlue+1);
    gr->SetLineWidth(2);
    
    gr->Draw("AP");
    
    TF1* fLin = new TF1("fLin", "[0] + [1]*x", -0.5, 5.5);
    fLin->SetLineColor(kRed);
    fLin->SetLineWidth(2);
    gr->Fit("fLin", "R");
    
    double p0 = fLin->GetParameter(0);
    double p1 = fLin->GetParameter(1);
    double ep0 = fLin->GetParError(0);
    double ep1 = fLin->GetParError(1);
    double chi2 = fLin->GetChisquare();
    int ndf = fLin->GetNDF();
    
    // Calcular métricas usando Q
    double sigma_Q_prom = 0;
    int n_valid = 0;
    for (size_t k = 0; k < errQ_vec.size(); k++) {
        if (errQ_vec[k] > 0 && errQ_vec[k] < 100) {
            sigma_Q_prom += errQ_vec[k];
            n_valid++;
        }
    }
    if (n_valid > 0) sigma_Q_prom /= n_valid;
    
    double precision = (std::abs(p1) > 1e-6) ? sigma_Q_prom / std::abs(p1) : 999;
    double LOD = 3.0 * precision;
    double LOQ = 10.0 * precision;
    
    // Caja de resultados
    TPaveText* pt = new TPaveText(0.50, 0.60, 0.88, 0.88, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(1);
    pt->SetTextAlign(12);
    pt->SetTextFont(42);
    pt->AddText("Modelo: Q = Q_{0} + k #cdot C_{REE}");
    pt->AddText(Form("Q_{0} = %.4f #pm %.4f", p0, ep0));
    pt->AddText(Form("k = %.4f #pm %.4f [1/%%]", p1, ep1));
    pt->AddText(Form("#chi^{2}/ndf = %.2f", (ndf > 0 ? chi2/ndf : 0)));
    pt->AddText("");
    pt->AddText(Form("Precision: #pm %.2f %% REE", precision));
    pt->AddText(Form("LOD (3#sigma): %.2f %% REE", LOD));
    pt->Draw();
    
    TLegend* leg = new TLegend(0.15, 0.15, 0.45, 0.35);
    leg->AddEntry(gr, "Datos GEANT4", "ep");
    leg->AddEntry(fLin, "Ajuste lineal", "l");
    leg->Draw();
    
    cCalib->SaveAs("Eu152_Calibracion.png");
    
    // =========================================================================
    // RESUMEN FINAL
    // =========================================================================
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  RESULTADOS FINALES (Indice Q)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    printf("\nLineas utilizadas:\n");
    printf("  Baja E (PE):  %.1f keV [ventana +/-%.0f keV]\n", E_LOW, ventana_low);
    printf("  Alta E (C):   %.1f keV [ventana +/-%.0f keV]\n", E_HIGH, ventana_high);
    
    printf("\nIndice: Q = N_low / N_high\n");
    printf("  Ventaja: No requiere logaritmos, mas estable numericamente\n");
    
    printf("\nModelo de calibracion:\n");
    printf("  Q = %.4f + (%.4f) * C_REE\n", p0, p1);
    printf("  chi2/ndf = %.2f\n", (ndf > 0 ? chi2/ndf : 0));
    
    printf("\nMetricas de desempeno:\n");
    printf("  Sensibilidad (k): %.4f +/- %.4f [Q/%% REE]\n", p1, ep1);
    printf("  Precision: +/- %.2f %% REE\n", precision);
    printf("  LOD (3 sigma): %.2f %% REE\n", LOD);
    printf("  LOQ (10 sigma): %.2f %% REE\n", LOQ);
    
    printf("\nPara muestra desconocida:\n");
    printf("  C_REE = (Q - %.4f) / (%.4f)\n", p0, p1);
    
    std::cout << "\nGraficos guardados:" << std::endl;
    std::cout << "  - Eu152_Espectros.png" << std::endl;
    std::cout << "  - Eu152_Transmisiones.png" << std::endl;
    std::cout << "  - Eu152_Comparacion_Indices.png" << std::endl;
    std::cout << "  - Eu152_Calibracion.png" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}
