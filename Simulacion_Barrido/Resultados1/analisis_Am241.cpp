#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TStyle.h"
#include "TMath.h"
#include "TPaveText.h"
#include <iostream>
#include <vector>
#include <cmath>

// --- FUNCIÓN AUXILIAR PARA OBTENER CUENTAS E INTEGRAR ---
double ObtenerCuentas(const char* nombreArchivo, double &error) {
    TFile* f = TFile::Open(nombreArchivo);
    if(!f || f->IsZombie()) {
        std::cerr << "[ERROR] No se pudo abrir: " << nombreArchivo << std::endl;
        return 0;
    }
    
    // NOTA: Ajusta "Energy" al nombre real de tu histograma en el .root
    TH1D* h = (TH1D*)f->Get("Energy"); 
    if(!h) { 
        // Intento de fallback por si GEANT4 usó nombres por defecto
        h = (TH1D*)f->Get("h1"); 
    }

    if(!h) {
        std::cerr << "[ERROR] Histograma no encontrado en " << nombreArchivo << std::endl;
        f->Close();
        return 0;
    }

    // Integramos todo el espectro. 
    // PEDAGOGÍA: En un escenario real, aquí definiríamos un ROI (Region of Interest)
    // solo alrededor del fotopico de 59.5 keV del Am-241 para evitar ruido Compton.
    double error_root = 0;
    double cuentas = h->IntegralAndError(1, h->GetNbinsX(), error_root);
    
    // ROOT calcula el error como sqrt(N) automáticamente para hisos sin pesos.
    error = error_root; 

    f->Close();
    return cuentas;
}

void analisis_Am241() {
    // --- 0. ESTILO GRÁFICO (El "Look" Profesional) ---
    gStyle->SetOptFit(1111); // Muestra: Chi2, Prob, Parametros, Errores
    gStyle->SetOptStat(0);   // Ocultamos la estadística del histograma base
    gStyle->SetLabelSize(0.04, "XY");
    gStyle->SetTitleSize(0.05, "XY");

    // --- 1. DATOS DE ENTRADA ---
    std::vector<double> conc = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; // % REE
    int n = conc.size();

    // Archivos de Am-241 (Sensibles al Z - Tierras Raras)
    std::vector<const char*> filesAm = {
        "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", 
        "Am241_3_REE.root", "Am241_4_REE.root", "Am241_5_REE.root"
    };

    // Vectores para almacenar datos procesados
    std::vector<double> I_Am(n), e_Am(n);   // Intensidad y error
    std::vector<double> T_Am(n), eT_Am(n);  // Transmisión y error propagado
    std::vector<double> x_err(n, 0.0);      // Error en X (asumimos concentración exacta en simulación)

    std::cout << "--- PROCESANDO DATOS SIMULADOS ---" << std::endl;

    // --- 2. LECTURA DE ARCHIVOS ---
    for(int i=0; i<n; i++) {
        I_Am[i] = ObtenerCuentas(filesAm[i], e_Am[i]);
        
        if (I_Am[i] <= 0) {
            std::cerr << "Advertencia: Cuentas 0 o negativas en " << filesAm[i] << std::endl;
            I_Am[i] = 1.0; // Evitar división por cero temporalmente
        }

        std::cout << "Conc: " << conc[i] << "% -> Cuentas: " << I_Am[i] 
                  << " +/- " << e_Am[i] << std::endl;
    }

    // --- 3. CÁLCULO DE TRANSMISIÓN Y PROPAGACIÓN DE ERROR ---
    // T = I(x) / I(0)
    // El punto 0 (0% REE) es nuestra referencia base I0.
    double I0 = I_Am[0];
    double eI0 = e_Am[0];

    for(int i=0; i<n; i++) {
        T_Am[i] = I_Am[i] / I0;

        // PEDAGOGÍA: Propagación de error para T = A / B
        // (dT/T)^2 = (dA/A)^2 + (dB/B)^2 - 2(cov_AB/AB)
        // Asumimos medidas independientes (covarianza = 0).
        // A = I_i, B = I_0.
        
        if (i == 0) {
            eT_Am[i] = 0.0; // Por definición T(0) = 1.0 sin error relativo a sí mismo en este contexto simplificado
        } else {
            double term1 = pow(e_Am[i] / I_Am[i], 2);
            double term2 = pow(eI0 / I0, 2);
            eT_Am[i] = T_Am[i] * sqrt(term1 + term2);
        }
    }

    // --- 4. GRAFICAR Y AJUSTAR ---
    TCanvas* c1 = new TCanvas("c1", "Curva de Calibracion REE", 900, 700);
    c1->SetGrid();
    c1->SetLeftMargin(0.12);
    c1->SetBottomMargin(0.12);

    // Crear el gráfico con errores (AQUÍ ESTÁ LA DISPERSIÓN)
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &T_Am[0], &x_err[0], &eT_Am[0]);
    
    gr->SetTitle("Sensibilidad Am-241 a Tierras Raras;Concentracion REE (%);Transmision Relativa (I/I_{0})");
    gr->SetMarkerStyle(21); // Cuadrados llenos
    gr->SetMarkerSize(1.2);
    gr->SetMarkerColor(kBlue+1);
    gr->SetLineColor(kBlue+1);
    gr->SetLineWidth(2);

    // Ajuste Exponencial: Ley de Beer-Lambert Modificada
    // T = p0 * exp(-p1 * x)
    // p0 debería ser cercano a 1. p1 es el coeficiente de sensibilidad másico efectivo.
    TF1* fFit = new TF1("fFit", "[0]*exp(-[1]*x)", 0, 5.5);
    fFit->SetParameters(1.0, 0.05); // Semillas iniciales lógicas
    fFit->SetParName(0, "Norm (T0)");
    fFit->SetParName(1, "Sensibilidad (k)");
    fFit->SetLineColor(kRed);

    gr->Fit("fFit", "R"); // "R" usa el rango de la función

    gr->Draw("AP"); // A=Ejes, P=Puntos con marcadores

    // --- 5. MOSTRAR LA ECUACIÓN EN PANTALLA ---
    // Obtenemos los parámetros ajustados
    double p0 = fFit->GetParameter(0);
    double p1 = fFit->GetParameter(1);
    double chi2 = fFit->GetChisquare();
    double ndf = fFit->GetNDF();

    TPaveText *pt = new TPaveText(0.4, 0.6, 0.85, 0.75, "NDC"); // Coordenadas normalizadas (0-1)
    pt->SetFillColor(0);
    pt->SetTextAlign(12);
    pt->AddText("Ley de Ajuste: T = I/I_{0} = p_{0} e^{-k #cdot C}");
    // Formateamos el string con los valores reales
    pt->AddText(Form("Ecuacion: T = %.3f e^{-%.4f #cdot C}", p0, p1));
    pt->AddText(Form("#chi^{2}/ndf = %.2f / %.0f = %.2f", chi2, ndf, chi2/ndf));
    pt->Draw();

    // Guardar
    c1->SaveAs("Curva_Calibracion_Am241.png");
}