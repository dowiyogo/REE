#include "TFile.h"
#include "TTree.h"
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

// Función para obtener cuentas desde TTree
Medicion ObtenerCuentas(const char* nombreArchivo) {
    TFile* f = TFile::Open(nombreArchivo);
    Medicion m = {0, 0};
    
    if(!f || f->IsZombie()) {
        std::cerr << "[ERROR] No abre: " << nombreArchivo << std::endl;
        return m;
    }
    
    TTree* t = (TTree*)f->Get("Scoring");
    if(t) {
        // Creamos histograma temp
        // Rango 0-3 MeV para cubrir Am y Na
        TH1D* h = new TH1D("hTemp", "Energy", 4096, 0, 3.0);
        t->Draw("Energy>>hTemp", "", "goff");
        
        // Integra todo (en analisis_LaBr3_ROI.cpp haremos ROIs especificos)
        double err_root;
        m.cuentas = h->IntegralAndError(1, h->GetNbinsX(), err_root);
        m.error = err_root;
        
        delete h;
    } else {
        std::cerr << "Tree Scoring no encontrado." << std::endl;
    }
    
    f->Close();
    return m;
}

void analisis_dual() {
    const int n = 6;
    double conc[n] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    
    // Archivos (Nombres coherentes con script de G4)
    std::vector<const char*> filesAm = {
        "Am241_0p0_REE.root", "Am241_0p01_REE.root", "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", "Am241_0p04_REE.root", "Am241_0p05_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0p0_REE.root", "Na22_0p01_REE.root", "Na22_0p02_REE.root", 
        "Na22_0p03_REE.root", "Na22_0p04_REE.root", "Na22_0p05_REE.root" 
    };

    std::vector<double> R_val(n), R_err(n), x_err(n, 0.0);
    Medicion Am0, Na0;

    // Obtener referencias (0%)
    Am0 = ObtenerCuentas(filesAm[0]);
    Na0 = ObtenerCuentas(filesNa[0]);

    std::cout << "--- ANALISIS DUAL-ENERGY (TTree) ---" << std::endl;
    std::cout << "I0(Am): " << Am0.cuentas << ", I0(Na): " << Na0.cuentas << std::endl;

    for(int i=0; i<n; i++) {
        Medicion Am = ObtenerCuentas(filesAm[i]);
        Medicion Na = ObtenerCuentas(filesNa[i]);

        // Transmisiones
        double T_Am = (Am0.cuentas > 0) ? Am.cuentas / Am0.cuentas : 0;
        double T_Na = (Na0.cuentas > 0) ? Na.cuentas / Na0.cuentas : 0;
        
        // Logaritmos (Atenuación)
        // Evitamos log(0)
        double L_Am = (T_Am > 0) ? -log(T_Am) : 0;
        double L_Na = (T_Na > 0) ? -log(T_Na) : 0;

        // Ratio R = ln(T_Am) / ln(T_Na)
        // Ojo con división por cero
        double R = (L_Na > 1e-5) ? L_Am / L_Na : 0;
        
        // --- PROPAGACIÓN DE ERROR SIMPLIFICADA PARA R ---
        // (Este es un paso crítico para tu memoria)
        // dR/R ~ sqrt( (dL_Am/L_Am)^2 + (dL_Na/L_Na)^2 )
        // donde dL = dT/T = dI/I (aprox Poisson 1/sqrt(N))
        
        double rel_err_Am = (Am.cuentas > 0) ? 1.0/sqrt(Am.cuentas) : 0;
        double rel_err_Na = (Na.cuentas > 0) ? 1.0/sqrt(Na.cuentas) : 0;
        
        // Error en los Logs
        double err_L_Am = rel_err_Am; // Porque d(ln x) = dx/x
        double err_L_Na = rel_err_Na;

        double term1 = (L_Am > 0) ? pow(err_L_Am/L_Am, 2) : 0;
        double term2 = (L_Na > 0) ? pow(err_L_Na/L_Na, 2) : 0;
        
        double err_R = (R > 0) ? R * sqrt(term1 + term2) : 0;

        // Caso base (0%) R es indeterminado matemáticamente (0/0), pero físicamente tiende a un límite.
        // Para graficar, lo pondremos como 0 o extrapolado, pero aquí lo dejamos tal cual.
        
        R_val[i] = R;
        R_err[i] = err_R;

        std::cout << "Conc: " << conc[i] << "% -> R: " << R << " +/- " << err_R << std::endl;
    }

    // Graficar R vs Concentración
    TCanvas* c1 = new TCanvas("c_dual", "Dual Energy", 800, 600);
    c1->SetGrid();
    
    // Omitimos el punto 0 para el ajuste si da problemas numéricos
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    gr->SetTitle("Curva de Calibracion Dual-Energy;Concentracion REE (%);Ratio R (ln T_{Am} / ln T_{Na})");
    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(kRed+1);
    gr->SetLineColor(kRed+1);

    TF1* fit = new TF1("fLin", "pol1", 0.5, 5.5); // Ajuste lineal ignorando el 0 exacto
    gr->Fit("fLin", "R");
    gr->Draw("AP");

    c1->SaveAs("Dual_Energy_Calibration.png");
}