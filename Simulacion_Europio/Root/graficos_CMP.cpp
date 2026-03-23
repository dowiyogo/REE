/**
 * @file graficos_CMP.cpp
 * @brief Generación de Gráficos Visuales para Presentación CMP
 * 
 * Crea gráficos claros y profesionales para audiencia industrial:
 *   1. Diagrama conceptual de la técnica
 *   2. Curva de calibración con zona de operación
 *   3. Comparación con concentraciones típicas de apatitos
 *   4. Gráfico de sensibilidad vs energía
 * 
 * Uso: root -l -b -q 'graficos_CMP.cpp'
 * 
 * @author Proyecto Medidor Tierras Raras
 * @date 2025
 */

#include <iostream>
#include <vector>
#include <cmath>

#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TH1F.h"
#include "TBox.h"
#include "TLine.h"
#include "TArrow.h"
#include "TEllipse.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TPad.h"
#include "TAxis.h"
#include "TPaveText.h"
#include "TROOT.h"

// ============================================================================
// Configuración de estilo global
// ============================================================================
void ConfigurarEstilo() {
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetPadGridX(false);
    gStyle->SetPadGridY(false);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
    gStyle->SetFrameLineWidth(2);
    gStyle->SetLineWidth(2);
    gStyle->SetHistLineWidth(2);
    gStyle->SetLegendBorderSize(0);
    gStyle->SetLegendFillColor(0);
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.05);
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetTitleSize(0.05, "XYZ");
    gStyle->SetLabelSize(0.04, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");
    gStyle->SetLabelFont(42, "XYZ");
}

// ============================================================================
// GRÁFICO 1: Diagrama Conceptual de la Técnica
// ============================================================================
void DiagramaConceptual() {
    TCanvas* c = new TCanvas("c_concepto", "Concepto Dual-Energy", 1200, 600);
    c->SetFillColor(kWhite);
    
    // Título
    TLatex* titulo = new TLatex(0.5, 0.92, "Principio de la T#acute{e}cnica Dual-Energy");
    titulo->SetNDC();
    titulo->SetTextAlign(22);
    titulo->SetTextSize(0.06);
    titulo->SetTextFont(62);
    titulo->Draw();
    
    // Fuente (izquierda)
    TEllipse* fuente = new TEllipse(0.12, 0.5, 0.06, 0.08);
    fuente->SetFillColor(kOrange+1);
    fuente->SetLineWidth(2);
    fuente->Draw();
    
    TLatex* txtFuente = new TLatex(0.12, 0.5, "Fuente");
    txtFuente->SetNDC();
    txtFuente->SetTextAlign(22);
    txtFuente->SetTextSize(0.03);
    txtFuente->SetTextFont(42);
    txtFuente->Draw();
    
    TLatex* txtFuente2 = new TLatex(0.12, 0.35, "Am-241 + Na-22");
    txtFuente2->SetNDC();
    txtFuente2->SetTextAlign(22);
    txtFuente2->SetTextSize(0.025);
    txtFuente2->SetTextColor(kOrange+2);
    txtFuente2->Draw();
    
    // Rayos gamma (flechas)
    TArrow* rayo1 = new TArrow(0.20, 0.55, 0.35, 0.55, 0.02, "|>");
    rayo1->SetLineColor(kRed);
    rayo1->SetFillColor(kRed);
    rayo1->SetLineWidth(3);
    rayo1->Draw();
    
    TLatex* txtRayo1 = new TLatex(0.275, 0.60, "59.5 keV");
    txtRayo1->SetNDC();
    txtRayo1->SetTextAlign(22);
    txtRayo1->SetTextSize(0.025);
    txtRayo1->SetTextColor(kRed);
    txtRayo1->Draw();
    
    TArrow* rayo2 = new TArrow(0.20, 0.45, 0.35, 0.45, 0.02, "|>");
    rayo2->SetLineColor(kBlue);
    rayo2->SetFillColor(kBlue);
    rayo2->SetLineWidth(3);
    rayo2->Draw();
    
    TLatex* txtRayo2 = new TLatex(0.275, 0.40, "511 keV");
    txtRayo2->SetNDC();
    txtRayo2->SetTextAlign(22);
    txtRayo2->SetTextSize(0.025);
    txtRayo2->SetTextColor(kBlue);
    txtRayo2->Draw();
    
    // Muestra (centro)
    TBox* muestra = new TBox(0.38, 0.35, 0.52, 0.65);
    muestra->SetFillColor(kGray+1);
    muestra->SetLineWidth(2);
    muestra->Draw();
    
    TLatex* txtMuestra = new TLatex(0.45, 0.50, "Apatito");
    txtMuestra->SetNDC();
    txtMuestra->SetTextAlign(22);
    txtMuestra->SetTextSize(0.035);
    txtMuestra->SetTextColor(kWhite);
    txtMuestra->SetTextFont(62);
    txtMuestra->Draw();
    
    TLatex* txtMuestra2 = new TLatex(0.45, 0.44, "+ REE");
    txtMuestra2->SetNDC();
    txtMuestra2->SetTextAlign(22);
    txtMuestra2->SetTextSize(0.03);
    txtMuestra2->SetTextColor(kYellow);
    txtMuestra2->SetTextFont(62);
    txtMuestra2->Draw();
    
    // Rayos atenuados
    TArrow* rayo1a = new TArrow(0.55, 0.55, 0.70, 0.55, 0.02, "|>");
    rayo1a->SetLineColor(kRed-7);
    rayo1a->SetFillColor(kRed-7);
    rayo1a->SetLineWidth(2);
    rayo1a->SetLineStyle(2);
    rayo1a->Draw();
    
    TArrow* rayo2a = new TArrow(0.55, 0.45, 0.70, 0.45, 0.02, "|>");
    rayo2a->SetLineColor(kBlue-7);
    rayo2a->SetFillColor(kBlue-7);
    rayo2a->SetLineWidth(2);
    rayo2a->Draw();
    
    // Detector (derecha)
    TBox* detector = new TBox(0.73, 0.35, 0.87, 0.65);
    detector->SetFillColor(kGreen+2);
    detector->SetLineWidth(2);
    detector->Draw();
    
    TLatex* txtDet = new TLatex(0.80, 0.50, "Detector");
    txtDet->SetNDC();
    txtDet->SetTextAlign(22);
    txtDet->SetTextSize(0.03);
    txtDet->SetTextColor(kWhite);
    txtDet->SetTextFont(62);
    txtDet->Draw();
    
    TLatex* txtDet2 = new TLatex(0.80, 0.44, "LaBr_{3}(Ce)");
    txtDet2->SetNDC();
    txtDet2->SetTextAlign(22);
    txtDet2->SetTextSize(0.025);
    txtDet2->SetTextColor(kWhite);
    txtDet2->Draw();
    
    // Cajas explicativas
    TPaveText* ptLow = new TPaveText(0.08, 0.12, 0.35, 0.25, "NDC");
    ptLow->SetFillColor(kRed-9);
    ptLow->SetBorderSize(1);
    ptLow->SetTextAlign(12);
    ptLow->AddText("#color[632]{Baja Energ#acute{i}a (59.5 keV)}");
    ptLow->AddText("Efecto Fotoel#acute{e}ctrico #propto Z^{4}");
    ptLow->AddText("#bf{Sensible a Tierras Raras}");
    ptLow->Draw();
    
    TPaveText* ptHigh = new TPaveText(0.40, 0.12, 0.67, 0.25, "NDC");
    ptHigh->SetFillColor(kBlue-9);
    ptHigh->SetBorderSize(1);
    ptHigh->SetTextAlign(12);
    ptHigh->AddText("#color[600]{Alta Energ#acute{i}a (511 keV)}");
    ptHigh->AddText("Efecto Compton #propto #rho");
    ptHigh->AddText("#bf{Sensible solo a Densidad}");
    ptHigh->Draw();
    
    TPaveText* ptRatio = new TPaveText(0.72, 0.12, 0.95, 0.25, "NDC");
    ptRatio->SetFillColor(kGreen-9);
    ptRatio->SetBorderSize(1);
    ptRatio->SetTextAlign(12);
    ptRatio->AddText("#color[418]{Ratio Q = A_{low}/A_{high}}");
    ptRatio->AddText("Cancela efecto de densidad");
    ptRatio->AddText("#bf{Mide SOLO concentraci#acute{o}n REE}");
    ptRatio->Draw();
    
    c->SaveAs("CMP_01_Diagrama_Conceptual.png");
    std::cout << "[OK] Generado: CMP_01_Diagrama_Conceptual.png" << std::endl;
}

// ============================================================================
// GRÁFICO 2: Curva de Calibración con Zona de Operación
// ============================================================================
void CurvaCalibracion() {
    TCanvas* c = new TCanvas("c_calib", "Curva de Calibracion", 1000, 700);
    c->SetGrid();
    c->SetLeftMargin(0.14);
    c->SetBottomMargin(0.14);
    
    // Datos de la simulación (del CSV)
    const int n = 10;
    double conc[n] = {0, 0.2, 0.4, 0.6, 0.8, 1.0, 2.0, 3.0, 4.0, 5.0};
    double Q[n] = {1.299, 1.266, 1.217, 1.157, 1.114, 1.064, 0.849, 0.680, 0.541, 0.438};
    double errQ[n] = {0.012, 0.014, 0.013, 0.010, 0.010, 0.012, 0.010, 0.009, 0.007, 0.007};
    
    TGraphErrors* gr = new TGraphErrors(n, conc, Q, nullptr, errQ);
    gr->SetMarkerStyle(21);
    gr->SetMarkerSize(1.5);
    gr->SetMarkerColor(kBlue+1);
    gr->SetLineColor(kBlue+1);
    gr->SetLineWidth(2);
    
    // Crear frame
    TH1F* frame = c->DrawFrame(-0.3, 0.3, 5.5, 1.5);
    frame->GetXaxis()->SetTitle("Concentraci#acute{o}n de Tierras Raras (%)");
    frame->GetYaxis()->SetTitle("Ratio Q = A_{low} / A_{high}");
    frame->GetXaxis()->SetTitleSize(0.05);
    frame->GetYaxis()->SetTitleSize(0.05);
    frame->GetXaxis()->SetTitleOffset(1.1);
    frame->GetYaxis()->SetTitleOffset(1.2);
    
    // Zona de operación lineal (sombreada)
    TBox* zonaLineal = new TBox(0.2, 0.3, 3.5, 1.5);
    zonaLineal->SetFillColor(kGreen-9);
    zonaLineal->SetFillStyle(3004);
    zonaLineal->Draw();
    
    // Zona de detección (LOD)
    TBox* zonaLOD = new TBox(0, 0.3, 0.2, 1.5);
    zonaLOD->SetFillColor(kRed-9);
    zonaLOD->SetFillStyle(3005);
    zonaLOD->Draw();
    
    // Ajuste lineal
    TF1* fit = new TF1("fit", "[0] + [1]*x", 0, 5);
    fit->SetParameters(1.3, -0.17);
    gr->Fit(fit, "Q", "", 0, 3.5);
    fit->SetLineColor(kRed);
    fit->SetLineWidth(3);
    fit->SetLineStyle(1);
    fit->Draw("same");
    
    // Dibujar puntos
    gr->Draw("P same");
    
    // Línea de LOD
    TLine* lodLine = new TLine(0.2, 0.3, 0.2, 1.5);
    lodLine->SetLineColor(kRed+1);
    lodLine->SetLineWidth(2);
    lodLine->SetLineStyle(2);
    lodLine->Draw();
    
    // Etiquetas
    TLatex* txtLOD = new TLatex(0.22, 1.35, "LOD #approx 2000 PPM");
    txtLOD->SetTextSize(0.035);
    txtLOD->SetTextColor(kRed+1);
    txtLOD->Draw();
    
    TLatex* txtZona = new TLatex(1.8, 1.42, "#bf{Zona de Operaci#acute{o}n Lineal}");
    txtZona->SetTextSize(0.04);
    txtZona->SetTextColor(kGreen+3);
    txtZona->SetTextAlign(22);
    txtZona->Draw();
    
    // Caja con parámetros
    TPaveText* pt = new TPaveText(0.55, 0.65, 0.92, 0.88, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(1);
    pt->SetTextAlign(12);
    pt->AddText("#bf{Modelo de Calibraci#acute{o}n}");
    pt->AddText(Form("Q = %.3f + (%.3f)#timesC_{REE}", fit->GetParameter(0), fit->GetParameter(1)));
    pt->AddText(Form("Sensibilidad: %.3f / %%REE", std::abs(fit->GetParameter(1))));
    pt->AddText(Form("#chi^{2}/ndf = %.2f", fit->GetChisquare()/fit->GetNDF()));
    pt->Draw();
    
    // Leyenda
    TLegend* leg = new TLegend(0.55, 0.50, 0.92, 0.63);
    leg->AddEntry(gr, "Datos Simulados (GEANT4)", "p");
    leg->AddEntry(fit, "Ajuste Lineal", "l");
    leg->Draw();
    
    // Título
    TLatex* titulo = new TLatex(0.5, 0.95, "Curva de Calibraci#acute{o}n: Ratio Q vs Concentraci#acute{o}n REE");
    titulo->SetNDC();
    titulo->SetTextAlign(22);
    titulo->SetTextSize(0.045);
    titulo->SetTextFont(62);
    titulo->Draw();
    
    c->SaveAs("CMP_02_Curva_Calibracion.png");
    std::cout << "[OK] Generado: CMP_02_Curva_Calibracion.png" << std::endl;
}

// ============================================================================
// GRÁFICO 3: Comparación con Apatitos Típicos
// ============================================================================
void ComparacionApatitos() {
    TCanvas* c = new TCanvas("c_apatitos", "Comparacion Apatitos", 1100, 700);
    c->SetLeftMargin(0.25);
    c->SetBottomMargin(0.12);
    c->SetTopMargin(0.10);
    
    // Datos de concentraciones típicas (en PPM)
    const int n = 6;
    const char* nombres[n] = {
        "Apatito com#acute{u}n",
        "Kiruna (Suecia)",
        "Carmen (Chile)",
        "El Laco (Chile)",
        "Apatito enriquecido",
        "L#acute{i}mite Detecci#acute{o}n"
    };
    double conc_min[n] = {500, 3000, 5000, 8000, 15000, 2000};
    double conc_max[n] = {2000, 8000, 12000, 20000, 50000, 2000};
    
    // Colores
    int colores[n] = {kGray+1, kBlue, kGreen+2, kOrange+1, kMagenta+1, kRed};
    
    // Crear histograma base
    TH1F* frame = new TH1F("frame", "", n, 0, n);
    frame->GetYaxis()->SetTitle("Concentraci#acute{o}n REE (PPM)");
    frame->GetYaxis()->SetRangeUser(0, 55000);
    frame->GetYaxis()->SetTitleSize(0.045);
    frame->GetYaxis()->SetTitleOffset(1.8);
    frame->Draw();
    
    // Dibujar barras
    for (int i = 0; i < n; i++) {
        double x1 = i + 0.15;
        double x2 = i + 0.85;
        
        if (i == n-1) {  // LOD - línea horizontal
            TLine* lodLine = new TLine(x1, conc_min[i], x2, conc_max[i]);
            lodLine->SetLineColor(kRed);
            lodLine->SetLineWidth(4);
            lodLine->SetLineStyle(2);
            lodLine->Draw();
        } else {
            TBox* bar = new TBox(x1, conc_min[i], x2, conc_max[i]);
            bar->SetFillColor(colores[i]);
            bar->SetLineWidth(2);
            bar->Draw();
            
            // Etiqueta de rango
            TLatex* lbl = new TLatex(i + 0.5, conc_max[i] + 1500, 
                Form("%.0f-%.0fk", conc_min[i]/1000.0, conc_max[i]/1000.0));
            lbl->SetTextAlign(22);
            lbl->SetTextSize(0.025);
            lbl->SetTextColor(colores[i]);
            lbl->Draw();
        }
    }
    
    // Nombres en el eje X (rotados)
    for (int i = 0; i < n; i++) {
        TLatex* nombre = new TLatex(i + 0.5, -3000, nombres[i]);
        nombre->SetTextAlign(32);
        nombre->SetTextAngle(45);
        nombre->SetTextSize(0.035);
        nombre->Draw();
    }
    
    // Zona detectable (sombreada)
    TBox* zonaDetectable = new TBox(0, 2000, n, 55000);
    zonaDetectable->SetFillColor(kGreen);
    zonaDetectable->SetFillStyle(3004);
    zonaDetectable->SetLineWidth(0);
    zonaDetectable->Draw();
    
    // Redibujar barras sobre zona
    for (int i = 0; i < n-1; i++) {
        double x1 = i + 0.15;
        double x2 = i + 0.85;
        TBox* bar = new TBox(x1, conc_min[i], x2, conc_max[i]);
        bar->SetFillColor(colores[i]);
        bar->SetLineWidth(2);
        bar->Draw();
    }
    
    // Línea de LOD
    TLine* lodFull = new TLine(0, 2000, n, 2000);
    lodFull->SetLineColor(kRed);
    lodFull->SetLineWidth(3);
    lodFull->SetLineStyle(2);
    lodFull->Draw();
    
    // Etiquetas
    TLatex* txtDetectable = new TLatex(n - 0.1, 35000, "#bf{ZONA DETECTABLE}");
    txtDetectable->SetTextAlign(32);
    txtDetectable->SetTextSize(0.04);
    txtDetectable->SetTextColor(kGreen+3);
    txtDetectable->Draw();
    
    TLatex* txtNoDetectable = new TLatex(n - 0.1, 1000, "No detectable");
    txtNoDetectable->SetTextAlign(32);
    txtNoDetectable->SetTextSize(0.03);
    txtNoDetectable->SetTextColor(kRed);
    txtNoDetectable->Draw();
    
    // Título
    TLatex* titulo = new TLatex(0.5, 0.95, "Concentraciones T#acute{i}picas de REE en Apatitos");
    titulo->SetNDC();
    titulo->SetTextAlign(22);
    titulo->SetTextSize(0.045);
    titulo->SetTextFont(62);
    titulo->Draw();
    
    // Subtítulo
    TLatex* subtitulo = new TLatex(0.5, 0.90, "Comparaci#acute{o}n con L#acute{i}mite de Detecci#acute{o}n del Sistema");
    subtitulo->SetNDC();
    subtitulo->SetTextAlign(22);
    subtitulo->SetTextSize(0.035);
    subtitulo->Draw();
    
    c->SaveAs("CMP_03_Comparacion_Apatitos.png");
    std::cout << "[OK] Generado: CMP_03_Comparacion_Apatitos.png" << std::endl;
}

// ============================================================================
// GRÁFICO 4: Sensibilidad vs Energía (por qué dual-energy)
// ============================================================================
void SensibilidadEnergia() {
    TCanvas* c = new TCanvas("c_sensib", "Sensibilidad vs Energia", 1000, 700);
    c->SetGrid();
    c->SetLogx();
    c->SetLogy();
    
    // Datos aproximados de coeficiente de atenuación
    const int n = 20;
    double E[n], mu_apatito[n], mu_REE[n], ratio[n];
    
    for (int i = 0; i < n; i++) {
        E[i] = 10 * std::pow(10, 2.3 * i / (n-1));  // 10 keV a 2000 keV
        
        // Aproximaciones simplificadas
        double E_keV = E[i];
        
        // Apatito (Z_eff ~ 15): σ_PE ∝ Z^4/E^3, σ_C ∝ ρ
        double sigma_PE_apat = std::pow(15, 4) / std::pow(E_keV, 3);
        double sigma_C_apat = 1.0;
        mu_apatito[i] = sigma_PE_apat + sigma_C_apat;
        
        // REE (Z_eff ~ 60): mayor efecto fotoeléctrico
        double sigma_PE_REE = std::pow(60, 4) / std::pow(E_keV, 3);
        double sigma_C_REE = 1.0;
        mu_REE[i] = sigma_PE_REE + sigma_C_REE;
        
        ratio[i] = mu_REE[i] / mu_apatito[i];
    }
    
    // Normalizar para visualización
    double max_apat = mu_apatito[0];
    double max_REE = mu_REE[0];
    for (int i = 0; i < n; i++) {
        mu_apatito[i] /= max_apat;
        mu_REE[i] /= max_REE;
    }
    
    TGraph* grApat = new TGraph(n, E, mu_apatito);
    grApat->SetLineColor(kBlue);
    grApat->SetLineWidth(3);
    grApat->SetLineStyle(2);
    
    TGraph* grREE = new TGraph(n, E, mu_REE);
    grREE->SetLineColor(kRed);
    grREE->SetLineWidth(3);
    
    TGraph* grRatio = new TGraph(n, E, ratio);
    grRatio->SetLineColor(kGreen+2);
    grRatio->SetLineWidth(4);
    
    // Dibujar
    TH1F* frame = c->DrawFrame(8, 0.001, 2500, 100);
    frame->GetXaxis()->SetTitle("Energ#acute{i}a (keV)");
    frame->GetYaxis()->SetTitle("Coeficiente de Atenuaci#acute{o}n (u.a.)");
    frame->GetXaxis()->SetTitleSize(0.045);
    frame->GetYaxis()->SetTitleSize(0.045);
    
    grApat->Draw("L same");
    grREE->Draw("L same");
    
    // Marcar energías de interés
    TLine* lineAm = new TLine(59.5, 0.001, 59.5, 100);
    lineAm->SetLineColor(kMagenta);
    lineAm->SetLineWidth(2);
    lineAm->SetLineStyle(2);
    lineAm->Draw();
    
    TLine* lineNa = new TLine(511, 0.001, 511, 100);
    lineNa->SetLineColor(kCyan+1);
    lineNa->SetLineWidth(2);
    lineNa->SetLineStyle(2);
    lineNa->Draw();
    
    // Etiquetas
    TLatex* txtAm = new TLatex(65, 50, "Am-241");
    txtAm->SetTextSize(0.035);
    txtAm->SetTextColor(kMagenta);
    txtAm->SetTextAngle(90);
    txtAm->Draw();
    
    TLatex* txtNa = new TLatex(550, 50, "Na-22");
    txtNa->SetTextSize(0.035);
    txtNa->SetTextColor(kCyan+1);
    txtNa->SetTextAngle(90);
    txtNa->Draw();
    
    // Zona de alta sensibilidad
    TBox* zonaAlta = new TBox(8, 0.001, 100, 100);
    zonaAlta->SetFillColor(kYellow);
    zonaAlta->SetFillStyle(3004);
    zonaAlta->Draw();
    
    grApat->Draw("L same");
    grREE->Draw("L same");
    
    // Leyenda
    TLegend* leg = new TLegend(0.55, 0.70, 0.92, 0.88);
    leg->AddEntry(grApat, "Apatito (Z_{eff} #approx 15)", "l");
    leg->AddEntry(grREE, "Tierras Raras (Z_{eff} #approx 60)", "l");
    leg->Draw();
    
    // Caja explicativa
    TPaveText* pt = new TPaveText(0.15, 0.15, 0.50, 0.35, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(1);
    pt->AddText("#bf{Zona de Alta Sensibilidad}");
    pt->AddText("E < 100 keV");
    pt->AddText("Efecto fotoel#acute{e}ctrico #propto Z^{4}");
    pt->AddText("Gran diferencia entre REE y matriz");
    pt->Draw();
    
    // Título
    TLatex* titulo = new TLatex(0.5, 0.95, "Fundamento F#acute{i}sico: Sensibilidad al N#acute{u}mero At#acute{o}mico");
    titulo->SetNDC();
    titulo->SetTextAlign(22);
    titulo->SetTextSize(0.045);
    titulo->SetTextFont(62);
    titulo->Draw();
    
    c->SaveAs("CMP_04_Sensibilidad_Energia.png");
    std::cout << "[OK] Generado: CMP_04_Sensibilidad_Energia.png" << std::endl;
}

// ============================================================================
// GRÁFICO 5: Resumen Visual de Resultados
// ============================================================================
void ResumenResultados() {
    TCanvas* c = new TCanvas("c_resumen", "Resumen Resultados", 1200, 600);
    c->Divide(2, 1);
    
    // Panel izquierdo: Z-score
    c->cd(1);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.15);
    
    const int n = 6;
    double conc[n] = {0, 0.2, 0.4, 0.6, 0.8, 1.0};
    double zscore[n] = {0.36, -2.44, -6.60, -11.70, -15.40, -19.63};
    
    TGraph* grZ = new TGraph(n, conc, zscore);
    grZ->SetMarkerStyle(21);
    grZ->SetMarkerSize(1.5);
    grZ->SetMarkerColor(kBlue+1);
    grZ->SetLineColor(kBlue+1);
    grZ->SetLineWidth(2);
    
    TH1F* frame1 = gPad->DrawFrame(-0.05, -22, 1.1, 5);
    frame1->GetXaxis()->SetTitle("C_{REE} (%)");
    frame1->GetYaxis()->SetTitle("Z-score");
    frame1->GetXaxis()->SetTitleSize(0.05);
    frame1->GetYaxis()->SetTitleSize(0.05);
    
    // Zona de no detección (|Z| < 3)
    TBox* zonaNoDet = new TBox(-0.05, -3, 1.1, 3);
    zonaNoDet->SetFillColor(kRed-9);
    zonaNoDet->SetFillStyle(1001);
    zonaNoDet->Draw();
    
    // Líneas de significancia
    TLine* l3p = new TLine(-0.05, 3, 1.1, 3);
    l3p->SetLineColor(kGreen+2);
    l3p->SetLineWidth(2);
    l3p->SetLineStyle(2);
    l3p->Draw();
    
    TLine* l3n = new TLine(-0.05, -3, 1.1, -3);
    l3n->SetLineColor(kGreen+2);
    l3n->SetLineWidth(2);
    l3n->SetLineStyle(2);
    l3n->Draw();
    
    grZ->Draw("LP same");
    
    TLatex* txt3sigma = new TLatex(0.95, 3.5, "3#sigma");
    txt3sigma->SetTextSize(0.04);
    txt3sigma->SetTextColor(kGreen+2);
    txt3sigma->Draw();
    
    TLatex* txtLOD = new TLatex(0.15, 1.5, "#bf{NO DETECTABLE}");
    txtLOD->SetTextSize(0.045);
    txtLOD->SetTextColor(kRed+1);
    txtLOD->Draw();
    
    TLatex* titulo1 = new TLatex(0.5, 0.93, "#bf{Significancia Estad#acute{i}stica}");
    titulo1->SetNDC();
    titulo1->SetTextAlign(22);
    titulo1->SetTextSize(0.055);
    titulo1->Draw();
    
    // Panel derecho: Métricas clave
    c->cd(2);
    gPad->SetLeftMargin(0.05);
    gPad->SetRightMargin(0.05);
    
    TPaveText* pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(2);
    pt->SetTextAlign(12);
    pt->SetTextSize(0.045);
    
    pt->AddText("#bf{M#acute{E}TRICAS CLAVE}");
    pt->AddText("");
    pt->AddText("#color[418]{#bf{L#acute{i}mite de Detecci#acute{o}n (LOD)}}");
    pt->AddText("   #approx 2000 PPM (0.2% REE)");
    pt->AddText("");
    pt->AddText("#color[418]{#bf{Sensibilidad}}");
    pt->AddText("   #Delta Q / #Delta C = 0.17 / %REE");
    pt->AddText("");
    pt->AddText("#color[418]{#bf{Rango Lineal}}");
    pt->AddText("   0.2% - 3.5% REE");
    pt->AddText("");
    pt->AddText("#color[418]{#bf{Precisi#acute{o}n (1% REE)}}");
    pt->AddText("   #pm 0.1% REE");
    pt->AddText("");
    pt->AddText("#color[600]{#bf{Validado con simulaci#acute{o}n GEANT4}}");
    pt->Draw();
    
    TLatex* titulo2 = new TLatex(0.5, 0.93, "#bf{Desempe#tilde{n}o del Sistema}");
    titulo2->SetNDC();
    titulo2->SetTextAlign(22);
    titulo2->SetTextSize(0.055);
    titulo2->Draw();
    
    c->SaveAs("CMP_05_Resumen_Resultados.png");
    std::cout << "[OK] Generado: CMP_05_Resumen_Resultados.png" << std::endl;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================
void graficos_CMP() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  GENERANDO GRAFICOS PARA CMP" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    ConfigurarEstilo();
    
    DiagramaConceptual();
    CurvaCalibracion();
    ComparacionApatitos();
    SensibilidadEnergia();
    ResumenResultados();
    
    std::cout << "\n[COMPLETADO] 5 graficos generados" << std::endl;
    std::cout << "Archivos: CMP_01_*.png a CMP_05_*.png\n" << std::endl;
}
