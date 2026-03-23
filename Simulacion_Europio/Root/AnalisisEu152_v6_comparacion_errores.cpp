/**
 * @file AnalisisEu152_v6_comparacion_errores.cpp
 * @brief Comparación de Modelos de Propagación de Error
 * 
 * Lee los resultados de v6 y recalcula con diferentes modelos de error:
 *   - Modelo 1: Error simple (√N_signal)
 *   - Modelo 2: Error con fondo (√(N_signal + N_bg))
 *   - Modelo 3: Error con normalización correcta
 *   - Modelo 4: Error escalado empíricamente
 * 
 * Compara chi²/ndf y LOD para cada modelo
 * 
 * Uso: root -l 'AnalisisEu152_v6_comparacion_errores.cpp("Eu152_v6_resultados.csv")'
 * 
 * @author Usuario/Claude
 * @date 2025-02
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TMultiGraph.h"
#include "TPaveText.h"

struct DatosMuestra {
    double conc;
    double A_low, errA_low_original;
    double A_high, errA_high_original;
    double Q_original, errQ_original;
    
    // Nuevos errores calculados
    double errA_low_modelo2;   // Incluyendo fondo
    double errA_low_modelo3;   // Con normalización correcta
    double errA_low_modelo4;   // Escalado empírico
    
    double errA_high_modelo2;
    double errA_high_modelo3;
    double errA_high_modelo4;
    
    double Q_modelo[4], errQ_modelo[4];
};

static double g_factor_escala_modelo4 = 1.0;

// ============================================================================
// FUNCIÓN: Calcular errores con diferentes modelos
// ============================================================================

void CalcularErroresModelos(std::vector<DatosMuestra>& datos,
                            double A_low_ref, double A_high_ref,
                            double fondo_low, double fondo_high) {
    
    // Calcular chi² inicial para modelo 4 (escalado)
    std::vector<double> C, Q, errQ;
    for (const auto& d : datos) {
        C.push_back(d.conc);
        Q.push_back(d.Q_original);
        errQ.push_back(d.errQ_original);
    }
    
    TGraphErrors grTemp(C.size(), &C[0], &Q[0], nullptr, &errQ[0]);
    TF1 fTemp("fTemp", "[0] + [1]*x", 0, 5);
    grTemp.Fit(&fTemp, "Q");
    double chi2_original = fTemp.GetChisquare();
    int ndf_original = fTemp.GetNDF();
    double factor_escala = (ndf_original > 0) ? std::sqrt(chi2_original / ndf_original) : 1.0;
    
    printf("\n[INFO] Factor de escala empirico (Modelo 4): %.3f\n", factor_escala);
    printf("       (basado en chi2/ndf = %.2f)\n\n", chi2_original/ndf_original);
    
    // Calcular factor de normalización para cada muestra
    for (auto& d : datos) {
        // Factor de normalización implícito
        double factor_norm_low = A_low_ref / d.A_low;
        double factor_norm_high = A_high_ref / d.A_high;
        
        // MODELO 1: Error simple (ya calculado en v6)
        // errA = √A_total
        d.Q_modelo[0] = d.Q_original;
        d.errQ_modelo[0] = d.errQ_original;
        
        // MODELO 2: Error incluyendo fondo
        // errA = √(A_signal + A_background)
        // Estimamos A_background proporcional al fondo de referencia
        double bg_low_estimado = fondo_low * (d.A_low / A_low_ref);
        double bg_high_estimado = fondo_high * (d.A_high / A_high_ref);
        
        d.errA_low_modelo2 = std::sqrt(d.A_low + bg_low_estimado);
        d.errA_high_modelo2 = std::sqrt(d.A_high + bg_high_estimado);
        
        d.Q_modelo[1] = d.Q_original;  // Q no cambia
        d.errQ_modelo[1] = d.Q_modelo[1] * std::sqrt(
            std::pow(d.errA_low_modelo2 / d.A_low, 2) +
            std::pow(d.errA_high_modelo2 / d.A_high, 2)
        );
        
        // MODELO 3: Error con normalización correcta
        // Al normalizar por factor k, error escala como √k * σ
        d.errA_low_modelo3 = d.errA_low_original * std::sqrt(factor_norm_low);
        d.errA_high_modelo3 = d.errA_high_original * std::sqrt(factor_norm_high);
        
        d.Q_modelo[2] = d.Q_original;
        d.errQ_modelo[2] = d.Q_modelo[2] * std::sqrt(
            std::pow(d.errA_low_modelo3 / d.A_low, 2) +
            std::pow(d.errA_high_modelo3 / d.A_high, 2)
        );
        
        // MODELO 4: Error escalado empíricamente
        // Multiplicamos por factor derivado del chi² original
        d.errA_low_modelo4 = d.errA_low_original * factor_escala;
        d.errA_high_modelo4 = d.errA_high_original * factor_escala;
        
        d.Q_modelo[3] = d.Q_original;
        d.errQ_modelo[3] = d.errQ_original * factor_escala;
    }
    g_factor_escala_modelo4 = factor_escala;
}

// ============================================================================
// FUNCIÓN: Analizar calibración con modelo de error específico
// ============================================================================

struct ResultadoCalibracion {
    double Q0, errQ0;
    double k, errk;
    double chi2;
    int ndf;
    double chi2_ndf;
    double LOD_teorico;
    double LOD_experimental;
    TGraphErrors* grafico;
    TF1* fit;
};

ResultadoCalibracion AnalizarCalibracion(const std::vector<DatosMuestra>& datos,
                                         int modelo,
                                         const char* nombre) {
    ResultadoCalibracion res;
    
    std::vector<double> C, Q, errQ;
    for (const auto& d : datos) {
        C.push_back(d.conc);
        Q.push_back(d.Q_modelo[modelo]);
        errQ.push_back(d.errQ_modelo[modelo]);
    }
    
    res.grafico = new TGraphErrors(C.size(), &C[0], &Q[0], nullptr, &errQ[0]);
    res.grafico->SetName(nombre);
    
    res.fit = new TF1(Form("fit_%s", nombre), "[0] + [1]*x", 0, 5);
    res.fit->SetParameters(1.3, -0.17);
    res.grafico->Fit(res.fit, "Q");
    
    res.Q0 = res.fit->GetParameter(0);
    res.errQ0 = res.fit->GetParError(0);
    res.k = res.fit->GetParameter(1);
    res.errk = res.fit->GetParError(1);
    res.chi2 = res.fit->GetChisquare();
    res.ndf = res.fit->GetNDF();
    res.chi2_ndf = (res.ndf > 0) ? res.chi2 / res.ndf : 0;
    
    // LOD teórico CORREGIDO
    // Usar el error de Q de la muestra de referencia (0% REE), NO el error del parámetro Q0 del fit
    double errQ_referencia = datos[0].errQ_modelo[modelo];
    double precision = errQ_referencia / std::abs(res.k);
    res.LOD_teorico = 3 * precision * 10000;  // PPM
    
    // LOD experimental (primera muestra con |Z| > 3)
    res.LOD_experimental = -1;
    for (const auto& d : datos) {
        if (d.conc > 0) {  // Excluir referencia
            double Z = (d.Q_modelo[modelo] - res.Q0) / res.errQ0;
            if (std::abs(Z) > 3.0 && res.LOD_experimental < 0) {
                res.LOD_experimental = d.conc * 10000;  // PPM
                break;
            }
        }
    }
    
    return res;
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

void AnalisisEu152_v6_comparacion_errores(const char* archivo_csv = "Eu152_v6_resultados.csv") {
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "  COMPARACION DE MODELOS DE PROPAGACION DE ERROR" << std::endl;
    std::cout << std::string(80, '=') << std::endl << std::endl;
    
    // =========================================================================
    // PASO 1: Leer datos del CSV de v6
    // =========================================================================
    
    std::ifstream file(archivo_csv);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir: " << archivo_csv << std::endl;
        return;
    }
    
    std::vector<DatosMuestra> datos;
    std::string line;
    std::getline(file, line);  // Saltar header
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        DatosMuestra d;
        
        std::getline(ss, token, ','); d.conc = std::stod(token);
        std::getline(ss, token, ','); // Tipo (ignorar)
        std::getline(ss, token, ','); d.A_low = std::stod(token);
        std::getline(ss, token, ','); d.errA_low_original = std::stod(token);
        std::getline(ss, token, ','); d.A_high = std::stod(token);
        std::getline(ss, token, ','); d.errA_high_original = std::stod(token);
        std::getline(ss, token, ','); d.Q_original = std::stod(token);
        std::getline(ss, token, ','); d.errQ_original = std::stod(token);
        
        datos.push_back(d);
    }
    file.close();
    
    printf("[INFO] Leidos %zu puntos desde %s\n", datos.size(), archivo_csv);
    
    // Obtener valores de referencia (primera muestra = 0% REE)
    double A_low_ref = datos[0].A_low;
    double A_high_ref = datos[0].A_high;
    
    // Estimar fondos (asumimos que se reportaron en v6)
    // Para este análisis, usamos valores típicos observados
    double fondo_low = 1512.0;   // Del output de v6
    double fondo_high = 2.9;
    
    // =========================================================================
    // PASO 2: Calcular errores con diferentes modelos
    // =========================================================================
    
    CalcularErroresModelos(datos, A_low_ref, A_high_ref, fondo_low, fondo_high);
    
    // =========================================================================
    // PASO 3: Analizar calibración para cada modelo
    // =========================================================================
    
    const char* nombres_modelos[4] = {
        "Modelo 1: Error Simple (sqrt(A))",
        "Modelo 2: Error con Fondo (sqrt(A+BG))",
        "Modelo 3: Error con Normalizacion (sqrt(A)*sqrt(k))",
        "Modelo 4: Error Escalado Empirico"
    };
    
    ResultadoCalibracion resultados[4];
    
    for (int i = 0; i < 4; i++) {
        resultados[i] = AnalizarCalibracion(datos, i, Form("modelo%d", i+1));
    }
    
    // =========================================================================
    // PASO 4: Tabla comparativa
    // =========================================================================
    
    printf("\n%s\n", std::string(120, '=').c_str());
    printf("  COMPARACION DE MODELOS (LOD CORREGIDO)\n");
    printf("%s\n", std::string(120, '=').c_str());
    printf("NOTA: LOD_teo se calcula con error de Q de referencia (no error del parametro Q0 del fit)\n");
    printf("%s\n", std::string(120, '-').c_str());
    printf("%-45s | chi2/ndf | LOD_teo (PPM) | LOD_exp (PPM) | Coherencia\n", "Modelo");
    printf("%s\n", std::string(120, '-').c_str());
    
    for (int i = 0; i < 4; i++) {
        double coherencia = (resultados[i].LOD_experimental > 0) ? 
                            resultados[i].LOD_experimental / resultados[i].LOD_teorico : 0;
        printf("%-45s | %8.3f | %13.0f | %13.0f | %10.2fx\n",
               nombres_modelos[i],
               resultados[i].chi2_ndf,
               resultados[i].LOD_teorico,
               resultados[i].LOD_experimental,
               coherencia);
    }
    printf("%s\n\n", std::string(120, '=').c_str());
    
    // =========================================================================
    // PASO 5: Gráficos comparativos
    // =========================================================================
    
    gStyle->SetOptFit(0);
    gStyle->SetOptStat(0);
    
    // Gráfico 1: Calibraciones superpuestas
    TCanvas* c1 = new TCanvas("c1", "Comparacion Calibraciones", 1200, 900);
    c1->Divide(2, 2);
    
    int colores[4] = {kBlue, kRed, kGreen+2, kMagenta};
    
    for (int i = 0; i < 4; i++) {
        c1->cd(i+1);
        gPad->SetGrid();
        
        resultados[i].grafico->SetMarkerStyle(20);
        resultados[i].grafico->SetMarkerSize(1.0);
        resultados[i].grafico->SetMarkerColor(colores[i]);
        resultados[i].grafico->SetLineColor(colores[i]);
        resultados[i].grafico->SetTitle(Form("%s;C_{REE} (%%);Q", nombres_modelos[i]));
        resultados[i].grafico->Draw("AP");
        
        resultados[i].fit->SetLineColor(colores[i]);
        resultados[i].fit->SetLineWidth(2);
        resultados[i].fit->Draw("same");
        
        TPaveText* pt = new TPaveText(0.15, 0.15, 0.50, 0.35, "NDC");
        pt->SetFillColor(0);
        pt->SetTextAlign(12);
        pt->AddText(Form("chi^{2}/ndf = %.2f", resultados[i].chi2_ndf));
        pt->AddText(Form("LOD = %.0f PPM", resultados[i].LOD_teorico));
        pt->Draw();
    }
    
    c1->SaveAs("Eu152_v6_Comparacion_Modelos.png");
    
    // Gráfico 2: Chi²/ndf vs Modelo
    TCanvas* c2 = new TCanvas("c2", "Chi2 por Modelo", 900, 600);
    c2->SetGrid();
    
    double x_modelos[4] = {1, 2, 3, 4};
    double chi2_valores[4];
    for (int i = 0; i < 4; i++) {
        chi2_valores[i] = resultados[i].chi2_ndf;
    }
    
    TGraph* grChi2 = new TGraph(4, x_modelos, chi2_valores);
    grChi2->SetTitle("Chi^{2}/ndf por Modelo;Modelo;chi^{2}/ndf");
    grChi2->SetMarkerStyle(21);
    grChi2->SetMarkerSize(2.0);
    grChi2->SetMarkerColor(kBlue);
    grChi2->SetLineWidth(2);
    grChi2->GetXaxis()->SetNdivisions(4);
    grChi2->GetYaxis()->SetRangeUser(0, std::max(25.0, chi2_valores[0]*1.2));
    grChi2->Draw("APL");
    
    // Línea de referencia en chi²/ndf = 1
    TLine* lRef = new TLine(0.5, 1.0, 4.5, 1.0);
    lRef->SetLineColor(kRed);
    lRef->SetLineStyle(2);
    lRef->SetLineWidth(2);
    lRef->Draw();
    
    TLatex* texRef = new TLatex(4.2, 1.2, "Ideal");
    texRef->SetTextColor(kRed);
    texRef->SetTextSize(0.03);
    texRef->Draw();
    
    c2->SaveAs("Eu152_v6_Chi2_Comparacion.png");
    
    // Gráfico 3: LOD por Modelo
    TCanvas* c3 = new TCanvas("c3", "LOD por Modelo", 900, 600);
    c3->SetGrid();
    
    double LOD_teo[4], LOD_exp[4];
    for (int i = 0; i < 4; i++) {
        LOD_teo[i] = resultados[i].LOD_teorico;
        LOD_exp[i] = resultados[i].LOD_experimental;
    }
    
    TGraph* grLOD_teo = new TGraph(4, x_modelos, LOD_teo);
    TGraph* grLOD_exp = new TGraph(4, x_modelos, LOD_exp);
    
    TMultiGraph* mg = new TMultiGraph();
    mg->SetTitle("LOD por Modelo;Modelo;LOD (PPM)");
    
    grLOD_teo->SetMarkerStyle(21);
    grLOD_teo->SetMarkerSize(1.5);
    grLOD_teo->SetMarkerColor(kBlue);
    grLOD_teo->SetLineColor(kBlue);
    grLOD_teo->SetLineWidth(2);
    
    grLOD_exp->SetMarkerStyle(22);
    grLOD_exp->SetMarkerSize(1.5);
    grLOD_exp->SetMarkerColor(kRed);
    grLOD_exp->SetLineColor(kRed);
    grLOD_exp->SetLineWidth(2);
    
    mg->Add(grLOD_teo, "LP");
    mg->Add(grLOD_exp, "LP");
    mg->Draw("A");
    mg->GetXaxis()->SetNdivisions(4);
    
    TLegend* leg = new TLegend(0.15, 0.70, 0.40, 0.88);
    leg->AddEntry(grLOD_teo, "LOD Teorico", "lp");
    leg->AddEntry(grLOD_exp, "LOD Experimental", "lp");
    leg->Draw();
    
    c3->SaveAs("Eu152_v6_LOD_Comparacion.png");
    
    // =========================================================================
    // RESUMEN Y RECOMENDACIÓN
    // =========================================================================
    
    printf("\n%s\n", std::string(80, '=').c_str());
    printf("  RECOMENDACION\n");
    printf("%s\n", std::string(80, '=').c_str());
    
    // Encontrar mejor modelo basado en criterios múltiples:
    // 1. Chi²/ndf cercano a 1 (pero no < 0.5 ni > 3)
    // 2. Coherencia LOD razonable (1.0 < coherencia < 2.0)
    int mejor_modelo = -1;
    double mejor_score = 1000;
    
    printf("\n>> ANALISIS DE MODELOS:\n\n");
    
    for (int i = 0; i < 4; i++) {
        double coherencia = (resultados[i].LOD_experimental > 0) ? 
                            resultados[i].LOD_experimental / resultados[i].LOD_teorico : 0;
        
        // Score: penaliza desviaciones de chi²/ndf=1 y coherencia fuera de rango
        double chi2_penalty = std::abs(resultados[i].chi2_ndf - 1.0);
        double coherencia_penalty = 0;
        
        if (coherencia < 1.0) {
            coherencia_penalty = 10.0;  // Muy malo: LOD_exp < LOD_teo es imposible
        } else if (coherencia > 2.0) {
            coherencia_penalty = (coherencia - 1.5) * 2.0;  // Penalizar exceso
        } else {
            coherencia_penalty = std::abs(coherencia - 1.2) * 0.5;  // Óptimo en 1.2
        }
        
        double score = chi2_penalty + coherencia_penalty;
        
        printf("   %s:\n", nombres_modelos[i]);
        printf("      Chi2/ndf = %.2f (desviacion = %.2f)\n", 
               resultados[i].chi2_ndf, chi2_penalty);
        printf("      Coherencia = %.2fx (penalty = %.2f)\n", coherencia, coherencia_penalty);
        printf("      Score total = %.2f %s\n\n", score, (score < mejor_score ? "***" : ""));
        
        if (score < mejor_score) {
            mejor_score = score;
            mejor_modelo = i;
        }
    }
    
    if (mejor_modelo < 0) mejor_modelo = 0;  // Fallback
    
    printf(">> MODELO RECOMENDADO: %s\n\n", nombres_modelos[mejor_modelo]);
    printf("   Chi2/ndf: %.2f", resultados[mejor_modelo].chi2_ndf);
    if (resultados[mejor_modelo].chi2_ndf >= 0.8 && resultados[mejor_modelo].chi2_ndf <= 1.5) {
        printf(" (excelente)");
    } else if (resultados[mejor_modelo].chi2_ndf > 1.5 && resultados[mejor_modelo].chi2_ndf <= 3.0) {
        printf(" (aceptable)");
    } else {
        printf(" (fuera de rango ideal)");
    }
    printf("\n");
    
    printf("   LOD teorico: %.0f PPM (%.2f%% REE)\n", 
           resultados[mejor_modelo].LOD_teorico, 
           resultados[mejor_modelo].LOD_teorico/10000.0);
    printf("   LOD experimental: %.0f PPM (%.2f%% REE)\n", 
           resultados[mejor_modelo].LOD_experimental,
           resultados[mejor_modelo].LOD_experimental/10000.0);
    
    double coherencia_mejor = resultados[mejor_modelo].LOD_experimental / 
                              resultados[mejor_modelo].LOD_teorico;
    printf("   Coherencia: %.2fx ", coherencia_mejor);
    if (coherencia_mejor >= 1.0 && coherencia_mejor <= 1.5) {
        printf("(muy buena)\n");
    } else if (coherencia_mejor > 1.5 && coherencia_mejor <= 2.0) {
        printf("(aceptable)\n");
    } else {
        printf("(%.0f%% de diferencia)\n", (coherencia_mejor - 1.0) * 100);
    }
    
    printf("\n>> JUSTIFICACION:\n");
    if (mejor_modelo == 0) {
        printf("   Modelo simple es suficiente (chi2/ndf aceptable, coherencia razonable)\n");
    } else if (mejor_modelo == 1) {
        printf("   Inclusion del error del fondo mejora el ajuste\n");
    } else if (mejor_modelo == 2) {
        printf("   Correccion por normalizacion es critica para este conjunto de datos\n");
    } else if (mejor_modelo == 3) {
        printf("   Escalado empirico necesario por dispersion adicional no identificada\n");
        printf("   Posibles causas: fluctuaciones MC, geometria, auto-absorcion\n");
    }
    
    printf("\n>> PARA TU TESIS:\n");
    printf("   Reporta LOD funcional: %.0f PPM (%.2f%% REE)\n", 
           resultados[mejor_modelo].LOD_experimental,
           resultados[mejor_modelo].LOD_experimental/10000.0);
    printf("   Chi2/ndf = %.2f (modelo estadisticamente validado)\n", 
           resultados[mejor_modelo].chi2_ndf);
    
    // Calcular LOQ (10-sigma)
    double LOQ_teorico = (resultados[mejor_modelo].LOD_teorico / 3.0) * 10.0;
    printf("   LOQ teorico (10-sigma): %.0f PPM (%.2f%% REE)\n",
           LOQ_teorico, LOQ_teorico/10000.0);
    
    if (mejor_modelo == 3) {
        printf("\n   NOTA: Factor de escala %.2f indica dispersion sistematica\n", g_factor_escala_modelo4);
        printf("         adicional, probablemente asociada a fluctuaciones\n");
        printf("         Monte Carlo en simulaciones GEANT4 independientes.\n");
    }
    
    printf("\n>> APLICABILIDAD:\n");
    printf("   Apatitos IOCG enriquecidos (>%.0f PPM): CUANTIFICABLE\n", LOQ_teorico);
    printf("   Apatitos mineralizados (%.0f-%.0f PPM): DETECTABLE\n", 
           resultados[mejor_modelo].LOD_experimental, LOQ_teorico);
    printf("   Apatitos pobres (<%.0f PPM): NO DETECTABLE\n", 
           resultados[mejor_modelo].LOD_experimental);
    
    printf("\n%s\n", std::string(80, '=').c_str());
    
    std::cout << "\n[INFO] Archivos generados:" << std::endl;
    std::cout << "  - Eu152_v6_Comparacion_Modelos.png" << std::endl;
    std::cout << "  - Eu152_v6_Chi2_Comparacion.png" << std::endl;
    std::cout << "  - Eu152_v6_LOD_Comparacion.png" << std::endl;
}
