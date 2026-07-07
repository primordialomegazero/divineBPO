// PHI-I18N — Multi-Language Support
// Auto-detect, translate responses

#pragma once
#include <string>
#include <map>
#include <algorithm>

namespace divine {

class I18N {
private:
    std::map<std::string, std::map<std::string, std::string>> translations;
    std::string default_lang;
    
public:
    I18N() : default_lang("en") {
        // English (default)
        translations["en"]["welcome"] = "Welcome to Divine BPO. How can I help you today?";
        translations["en"]["listening"] = "I'm listening. Please tell me what you need.";
        translations["en"]["goodbye"] = "Thank you for calling. Goodbye.";
        translations["en"]["ticket_created"] = "Your ticket has been created. Reference: ";
        
        // Spanish
        translations["es"]["welcome"] = "Bienvenido a Divine BPO. ¿Cómo puedo ayudarle hoy?";
        translations["es"]["listening"] = "Estoy escuchando. Por favor, dígame qué necesita.";
        translations["es"]["goodbye"] = "Gracias por llamar. Adiós.";
        translations["es"]["ticket_created"] = "Su ticket ha sido creado. Referencia: ";
        
        // French
        translations["fr"]["welcome"] = "Bienvenue chez Divine BPO. Comment puis-je vous aider?";
        translations["fr"]["listening"] = "J'écoute. Dites-moi ce dont vous avez besoin.";
        translations["fr"]["goodbye"] = "Merci d'avoir appelé. Au revoir.";
        translations["fr"]["ticket_created"] = "Votre ticket a été créé. Référence: ";
        
        // Japanese
        translations["ja"]["welcome"] = "Divine BPOへようこそ。どのようにお手伝いできますか？";
        translations["ja"]["listening"] = "聞いています。必要なことを教えてください。";
        translations["ja"]["goodbye"] = "お電話ありがとうございます。さようなら。";
        translations["ja"]["ticket_created"] = "チケットが作成されました。参照番号: ";
        
        // Tagalog
        translations["tl"]["welcome"] = "Maligayang pagdating sa Divine BPO. Paano kita matutulungan ngayon?";
        translations["tl"]["listening"] = "Nakikinig ako. Sabihin mo kung ano ang kailangan mo.";
        translations["tl"]["goodbye"] = "Salamat sa pagtawag. Paalam.";
        translations["tl"]["ticket_created"] = "Nagawa na ang iyong ticket. Reference: ";
    }
    
    std::string detect_language(const std::string& text) {
        std::string lower = text;
        for (char& c : lower) c = tolower(c);
        
        // Spanish indicators
        if (lower.find("hola") != std::string::npos || lower.find("gracias") != std::string::npos || 
            lower.find("ayuda") != std::string::npos) return "es";
        
        // French indicators
        if (lower.find("bonjour") != std::string::npos || lower.find("merci") != std::string::npos ||
            lower.find("aide") != std::string::npos) return "fr";
        
        // Japanese indicators
        if (lower.find("konnichiwa") != std::string::npos || lower.find("arigatou") != std::string::npos ||
            lower.find("sumimasen") != std::string::npos) return "ja";
        
        // Tagalog indicators
        if (lower.find("magandang") != std::string::npos || lower.find("salamat") != std::string::npos ||
            lower.find("tulong") != std::string::npos || lower.find("paano") != std::string::npos) return "tl";
        
        return "en";
    }
    
    std::string translate(const std::string& key, const std::string& lang = "en") {
        if (translations.find(lang) != translations.end() &&
            translations[lang].find(key) != translations[lang].end()) {
            return translations[lang][key];
        }
        return translations["en"][key];
    }
    
    std::string auto_response(const std::string& key, const std::string& user_input) {
        std::string lang = detect_language(user_input);
        return translate(key, lang);
    }
};

} // namespace divine
