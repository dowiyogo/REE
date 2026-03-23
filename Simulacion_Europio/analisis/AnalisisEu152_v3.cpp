/**
 * @file AnalisisEu152_v3.cpp
 * @brief Análisis Dual-Energy con Eu-152 - CORREGIDO (Persistencia de Gráficos)
 * @author Tu Asistente Técnico
 * * FIX: Se añadió h->SetDirectory(0) para evitar que los histogramas se borren
 * al cerrar los archivos .root individuales.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TLine.h"
#include "TLatex.h"

// ============================================================================
// CONFIGURACIÓN - FÍSICA
// ============================================================================
const double E_122  = 121.78;  
const double E_344  = 344.28;  
const double E_779  = 778.90;  

// Selección de líneas para el índice Q
const double E_LOW  = E_122;   
const double E_HIGH = E_779;   

const double WIN_LOW  = 10.0;  
const double WIN_HIGH = 18.0;  

// Estructura para guardar datos
struct Muestra {
    std::string nombre_archivo;
    double concentracion_real;
    double N_low, err_N_low;
    double N_high, err_N_high;
    double Q, err_Q;
};

// ============================================================================
// FUNCIÓN DE INTEGRACIÓN
// ============================================================================
void IntegrarPico(TH1D* h, double E_centro, double semi_ancho, double &N_netas, double &error) {
    if (!h) return;
    int bin_min = h->FindBin(E_centro - semi_ancho);
    int bin_max = h->FindBin(E_centro + semi_ancho);
    double N_brutas = h->Integral(bin_min, bin_max);
    
    // Fondo
    int bg_L1 = h->FindBin(E_centro - 2.0*semi_ancho);
    int bg_L2 = h->FindBin(E_centro - 1.0*semi_ancho);
    int bg_R1 = h->FindBin(E_centro + 1.0*semi_ancho);
    int bg_R2 = h->FindBin(E_centro + 2.0*semi_ancho);
    
    double N_bg_counts = h->Integral(bg_L1, bg_L2) + h->Integral(bg_R1, bg_R2);
    int n_bins_pico = bin_max - bin_min + 1;
    int n_bins_bg = (bg_L2 - bg_L1 + 1) + (bg_R2 - bg_R1 + 1);
    
    double Fondo = (n_bins_bg > 0) ? N_bg_counts * (double)n_bins_pico / (double)n_bins_bg : 0;
    
    N_netas = N_brutas - Fondo;
    
    double ratio = (n_bins_bg > 0) ? (double)n_bins_pico / (double)n_bins_bg : 0;
    double var_fondo = ratio * ratio * N_bg_counts;
    error = std::sqrt(N_brutas + var_fondo); // Error propagado Poisson
}

// ============================================================================
// MAIN
// ============================================================================
void AnalisisEu152_v3() {
    
    gStyle->SetOptStat(0);
    gStyle->SetTitleSize(0.045, "XY");
    gStyle->SetLabelSize(0.035, "XY");
    gStyle->SetPalette(kRainBow); // Paleta de colores más clara
    
    std::cout << "=== ANALISIS Eu-152 v3 (CORREGIDO) ===" << std::endl;

    // Lista de archivos y concentración real
    std::vector<std::pair<std::string, double>> lista_archivos = {
        {"0p00",  0.0},
        {"0p002", 0.2},
        {"0p004", 0.4},
        {"0p006", 0.6},
        {"0p008", 0.8},
        {"0p01",  1.0},
        {"0p02",  2.0},
        {"0p03",  3.0},
        {"0p04",  4.0},
        {"0p05",  5.0}
    };
    
    std::vector<Muestra> resultados;
    
    // --- CANVAS 1: ESPECTROS ---
    TCanvas* cSpec = new TCanvas("cSpec", "Espectros Eu-152", 1000, 600);
    cSpec->SetLogy(); // Escala logarítmica para ver mejor los picos
    TLegend* legSpec = new TLegend(0.7, 0.6, 0.88, 0.88);
    legSpec->SetBorderSize(0);
    
    int color_idx = 0;
    int n_files = lista_archivos.size();

    // Contenedor para mantener los histogramas en memoria
    // Usamos una lista estática o vector de punteros para que no se pierdan
    static std::vector<TH1D*> hists_persistentes; 
    
    for (auto& item : lista_archivos) {
        std::string filename = "Eu152_REE_" + item.first + ".root";
        
        TFile* f = TFile::Open(filename.c_str());
        if (!f || f->IsZombie()) {
            std::cout << "SALTANDO: " << filename << " (No encontrado)" << std::endl;
            continue; 
        }
        
        TTree* t = (TTree*)f->Get("Scoring");
        std::string hname = "h_" + item.first;
        
        // Crear histograma
        TH1D* h = new TH1D(hname.c_str(), "Espectros de Energia (Eu-152);Energia [keV];Cuentas", 2000, 0, 2000);
        t->Draw(("Energy*1000>>" + hname).c_str(), "", "goff");
        
        // --- FIX CRÍTICO: DESVINCULAR DEL ARCHIVO ---
        h->SetDirectory(0); 
        // --------------------------------------------
        
        // Estética
        int col = kBlue - 9 + (color_idx * 2); // Gradiente azul a oscuro
        if (item.second == 0.0) col = kBlack;  // El 0% en negro
        if (item.second == 5.0) col = kRed;    // El máximo en rojo
        
        h->SetLineColor(col);
        h->SetLineWidth(2);
        
        // Guardar en vector persistente
        hists_persistentes.push_back(h);

        // Análisis Numérico
        Muestra m;
        m.nombre_archivo = filename;
        m.concentracion_real = item.second;
        IntegrarPico(h, E_LOW, WIN_LOW, m.N_low, m.err_N_low);
        IntegrarPico(h, E_HIGH, WIN_HIGH, m.N_high, m.err_N_high);
        
        if (m.N_high > 0) {
            m.Q = m.N_low / m.N_high;
            m.err_Q = m.Q * std::sqrt( pow(m.err_N_low/m.N_low, 2) + pow(m.err_N_high/m.N_high, 2) );
        } else { m.Q = 0; m.err_Q = 0; }
        resultados.push_back(m);
        
        // Dibujar (Ahora sí funcionará)
        cSpec->cd();
        if (color_idx == 0) {
            h->GetXaxis()->SetRangeUser(50, 900); // Zoom en zona de interés
            h->Draw("HIST");
        } else {
            h->Draw("HIST SAME");
        }
        
        if (item.second == 0.0 || item.second == 1.0 || item.second == 5.0)
            legSpec->AddEntry(h, Form("%.1f%% REE", item.second), "l");
            
        f->Close(); // Ahora es seguro cerrar
        color_idx++;
        
        printf(" %-15s | Q: %.4f +/- %.4f\n", item.first.c_str(), m.Q, m.err_Q);
    }
    legSpec->Draw();
    
    // --- CANVAS 2: CALIBRACIÓN ---
    int n = resultados.size();
    if (n > 0) {
        TCanvas* cCalib = new TCanvas("cCalib", "Curva de Calibracion", 900, 700);
        cCalib->SetGrid();
        
        std::vector<double> x(n), y(n), ex(n), ey(n);
        for(int i=0; i<n; i++) {
            x[i] = resultados[i].concentracion_real;
            y[i] = resultados[i].Q;
            ex[i] = 0;
            ey[i] = resultados[i].err_Q;
        }
        
        TGraphErrors* gr = new TGraphErrors(n, x.data(), y.data(), ex.data(), ey.data());
        gr->SetTitle("Curva de Calibracion (122 vs 779 keV);Concentracion REE [% Peso];Indice Q");
        gr->SetMarkerStyle(21);
        gr->SetMarkerColor(kRed+1);
        gr->Draw("AP");
        
        // Ajuste Lineal
        TF1* fLin = new TF1("fLin", "[0] + [1]*x", 0, 6.0);
        gr->Fit("fLin", "Q");
        
        double p0 = fLin->GetParameter(0);
        double p1 = fLin->GetParameter(1);
        double sigma_blank = resultados[0].err_Q; // Error del blanco
        
        // LOD y LOQ
        double LOD = 3.0 * sigma_blank / std::abs(p1);
        double LOQ = 10.0 * sigma_blank / std::abs(p1);
        
        TPaveText* pt = new TPaveText(0.5, 0.6, 0.88, 0.85, "NDC");
        pt->AddText(Form("Pendiente: %.4f", p1));
        pt->AddText(Form("Error (0%%): %.4f", sigma_blank));
        pt->AddText("--------------------");
        pt->AddText(Form("LOD: %.3f %%", LOD));
        pt->AddText(Form("LOQ: %.3f %%", LOQ));
        pt->Draw();
        
        std::cout << "\nRESULTADO FINAL:" << std::endl;
        std::cout << "LOD calculado: " << LOD << " %" << std::endl;
    }
}
