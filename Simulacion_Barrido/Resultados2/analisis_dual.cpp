#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TMath.h"
#include <iostream>
#include <vector>
#include <cmath>

// Estructura para devolver cuentas y su error
struct Medicion {
    double cuentas;
    double error;
};

// Función para obtener cuentas (Integra todo el espectro)
Medicion ObtenerCuentas(const char* nombreArchivo) {
    TFile* f = TFile::Open(nombreArchivo);
    Medicion m = {0, 0};
    
    if(!f || f->IsZombie()) {
        std::cerr << "[ERROR] No abre: " << nombreArchivo << std::endl;
        return m;
    }
    
    // Intenta buscar el histograma con nombres comunes
    TH1D* h = (TH1D*)f->Get("Energy"); 
    if(!h) h = (TH1D*)f->Get("h1");
    if(!h) h = (TH1D*)f->Get("fHistoSource");

    if(h) {
        // Integra todo. OJO: Para Na-22 idealmente usaríamos solo el fotopico,
        // pero para esta fase inicial usaremos la integral total para tener más estadística.
        double err_root;
        m.cuentas = h->IntegralAndError(1, h->GetNbinsX(), err_root);
        m.error = err_root;
    }
    f->Close();
    return m;
}

void analisis_dual() {
    gStyle->SetOptFit(111);
    gStyle->SetOptStat(0);

    // --- 1. CONFIGURACIÓN ---
    std::vector<double> conc = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; 
    const int n = conc.size();

    // Archivos (Asegúrate de que existan)
    std::vector<const char*> filesAm = {
        "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", 
        "Am241_3_REE.root", "Am241_4_REE.root", "Am241_5_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0_REE.root", "Na22_1_REE.root", "Na22_2_REE.root", 
        "Na22_3_REE.root", "Na22_4_REE.root", "Na22_5_REE.root" 
    };

    // Vectores de datos
    std::vector<double> R_val(n), R_err(n);
    std::vector<double> x_err(n, 0.0);

    // Referencias (I0) tomadas del primer punto (0% REE)
    Medicion Am0, Na0;
    
    std::cout << "--- LEITURA DE DATOS ---" << std::endl;
    for(int i=0; i<n; i++) {
        Medicion Am = ObtenerCuentas(filesAm[i]);
        Medicion Na = ObtenerCuentas(filesNa[i]);

        if(i==0) { Am0 = Am; Na0 = Na; }

        // --- CÁLCULO DE TRANSMISIONES ---
        double T_Am = Am.cuentas / Am0.cuentas;
        double T_Na = Na.cuentas / Na0.cuentas;

        // Evitar log(0) o divisiones por cero
        if(T_Am <= 1e-9) T_Am = 1e-9; 
        if(T_Na <= 1e-9) T_Na = 1e-9; // El Na-22 penetra bien, esto rara vez pasará

        // Logaritmos (Atenuación L = -ln(T))
        double L_Am = -log(T_Am);
        double L_Na = -log(T_Na);

        // --- CÁLCULO DEL RATIO R ---
        // R = L_Am / L_Na. 
        // Si la densidad cambia, L_Am y L_Na cambian proporcionalmente, R se mantiene cte (idealmente).
        // Si cambia REE (Z), L_Am cambia mucho, L_Na poco -> R cambia.
        
        double R = 0;
        if(L_Na > 1e-5) R = L_Am / L_Na;
        else R = 0; // Caso patológico (0% atenuación Na)

        // --- PROPAGACIÓN DE ERROR (La parte difícil) ---
        // sigma_L = dl/dT * sigma_T = (1/T) * T * sqrt(...) = sqrt(1/N + 1/N0)
        double term_Am = 1.0/Am.cuentas + 1.0/Am0.cuentas; // sigma_L_Am^2 aprox
        double term_Na = 1.0/Na.cuentas + 1.0/Na0.cuentas; // sigma_L_Na^2 aprox
        
        // Propagación para división R = A/B -> (dR/R)^2 = (dA/A)^2 + (dB/B)^2
        // dA = sqrt(term_Am), A = L_Am
        double err_rel_sq = (term_Am / (L_Am*L_Am)) + (term_Na / (L_Na*L_Na));
        double err_R = R * sqrt(err_rel_sq);

        // Caso especial i=0 (Referencia contra referencia)
        if(i==0) { R = 1.0; err_R = 0.05; } // Valor nominal para que no rompa el gráfico

        R_val[i] = R;
        R_err[i] = err_R;

        std::cout << "Conc: " << conc[i] << "% | R: " << R << " +/- " << err_R << std::endl;
    }

    // --- GRÁFICO ---
    TCanvas* c1 = new TCanvas("cDual", "Calibracion Dual", 900, 600);
    c1->SetGrid();
    
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    gr->SetTitle("Curva de Calibracion Dual-Energy;Concentracion REE (%);Ratio R (ln T_{Am} / ln T_{Na})");
    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(kRed+1);
    gr->SetLineColor(kRed+1);

    // Ajuste Lineal: R = p0 + p1 * Conc
    // (Para REE bajos, la relación R vs Conc suele ser lineal)
    TF1* fit = new TF1("fLin", "pol1", 0, 5); 
    gr->Fit("fLin", "R");
    gr->Draw("AP");

    // --- ANÁLISIS DE ERROR Y SESGO (INVERSE PROBLEM) ---
    // C_est = (R_medido - p0) / p1
    double p0 = fit->GetParameter(0);
    double p1 = fit->GetParameter(1);
    double ep0 = fit->GetParError(0);
    double ep1 = fit->GetParError(1);

    std::cout << "\n--- VALIDACIÓN INVERSA (ESTIMACIÓN) ---" << std::endl;
    std::cout << "Modelo Inverso: C = (R - " << p0 << ") / " << p1 << std::endl;
    
    for(int i=0; i<n; i++) {
        // 1. Estimación Puntual
        double C_est = (R_val[i] - p0) / p1;
        
        // 2. Sesgo (Bias)
        double bias = C_est - conc[i];

        // 3. Error Propagado en la Estimación (Sigma_C)
        // C = (R - p0)/p1. Solo propagamos error de R (el del instrumento)
        // dC/dR = 1/p1
        double sigma_C = R_err[i] / fabs(p1);

        std::cout << "Real: " << conc[i] << "% -> Est: " << C_est 
                  << " +/- " << sigma_C << "% | Sesgo: " << bias << "%" << std::endl;
    }
    
    c1->SaveAs("Calibracion_Dual.png");
}