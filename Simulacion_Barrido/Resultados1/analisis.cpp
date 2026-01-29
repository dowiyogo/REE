#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TMath.h"
#include <iostream>
#include <vector>

// Función auxiliar para integrar el pico (suma de cuentas)
double ObtenerCuentas(const char* nombreArchivo, double &error) {
    TFile* f = TFile::Open(nombreArchivo);
    if(!f || f->IsZombie()) {
        std::cerr << "Error abriendo " << nombreArchivo << std::endl;
        return 0;
    }
    
    // Obtenemos el histograma "Energy" (ID 0)
    TH1D* h = (TH1D*)f->Get("Energy"); // Ojo: en ROOT se guarda con nombre, no ID
    if(!h) {
        // A veces GEANT4 guarda como "Energy_h1" o similar, revisa con TBrowser
        // Probamos nombres comunes
        h = (TH1D*)f->Get("Energy"); 
        if(!h) h = (TH1D*)f->Get("h1");
    }

    if(!h) {
        std::cerr << "No se encontró el histograma en " << nombreArchivo << std::endl;
        f->Close();
        return 0;
    }

    // Integramos todo el histograma (o podrías definir un rango específico ROI)
    double cuentas = h->IntegralAndError(1, h->GetNbinsX(), error);
    
    f->Close();
    return cuentas;
}

void analisis() {
    // --- 1. DATOS DE ENTRADA ---
    // Concentraciones simuladas (%)
    std::vector<double> conc = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; // 0%, 1%, 2%, 3%, 4%, 5%
    int n = conc.size();

    // Archivos (Asegúrate de que coincidan con los nombres de tu macro)
    std::vector<const char*> filesAm = {
        "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", "Am241_3_REE.root","Am241_4_REE.root","Am241_5_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0_REE.root", "Na22_1_REE.root", "Na22_2_REE.root", "Na22_3_REE.root", "Na22_4_REE.root", "Na22_5_REE.root" 
        // ¡OJO! En tu macro no vi el de 2% para Na22, ajusta si falta
    };

    // Arrays para guardar resultados
    std::vector<double> I_Am(n), e_Am(n);
    
    // --- 2. LECTURA DE DATOS ---
    std::cout << "--- LECTURA DE RESULTADOS ---" << std::endl;
    for(int i=0; i<n; i++) {
        double err;
        I_Am[i] = ObtenerCuentas(filesAm[i], err);
        e_Am[i] = err; // Error estadístico (sqrt(N))
        
        std::cout << "Conc: " << conc[i] << "% -> Cuentas Am-241: " << I_Am[i] 
                  << " +/- " << e_Am[i] << std::endl;
    }

    // --- 3. CÁLCULO DE LA TRANSMISIÓN RELATIVA (T) ---
    // T = I(x) / I(0)
    std::vector<double> T_Am(n), eT_Am(n);
    for(int i=0; i<n; i++) {
        T_Am[i] = I_Am[i] / I_Am[0];
        // Propagación de error para división: f = A/B -> (df/f)^2 = (dA/A)^2 + (dB/B)^2
        double term1 = pow(e_Am[i]/I_Am[i], 2);
        double term2 = pow(e_Am[0]/I_Am[0], 2);
        eT_Am[i] = T_Am[i] * sqrt(term1 + term2);
    }

    // --- 4. GRAFICAR ---
    TCanvas* c1 = new TCanvas("c1", "Curva de Calibracion", 800, 600);
    c1->SetGrid();

    // Gráfico de Transmisión vs Concentración
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &T_Am[0], 0, &eT_Am[0]);
    gr->SetTitle("Sensibilidad Am-241 a Tierras Raras;Concentracion REE (%);Transmision Normalizada (I/I0)");
    gr->SetMarkerStyle(21);
    gr->SetMarkerColor(kBlue);
    gr->SetLineColor(kBlue);
    gr->SetLineWidth(2);
    gr->Draw("ALP");

    // Ajuste Exponencial (Ley de Beer-Lambert modificada)
    // T = exp(-mu_eff * C) -> Ajustamos funcion "exp([0] + [1]*x)"
    // Ojo: Como es relativa, debería partir de 1.
    TF1* fFit = new TF1("fFit", "exp([0] * x)", 0, 5);
    gr->Fit(fFit);

    std::cout << "--- RESULTADOS DEL AJUSTE ---" << std::endl;
    std::cout << "Pendiente de Atenuación (Sensibilidad): " << fFit->GetParameter(0) << std::endl;
    
    c1->SaveAs("Curva_Calibracion_REE.png");
}

