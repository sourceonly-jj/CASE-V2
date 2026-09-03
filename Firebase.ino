// ==========================
// Firebase Anonymous Auth
// ==========================

bool firebaseAnonymousSignIn() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Auth failed: no WiFi");
    return false;
  }

  HTTPClient http;

  String url =
    "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" +
    String(FIREBASE_API_KEY);

  String payload = "{\"returnSecureToken\":true}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);

  Serial.print("Firebase auth code: ");
  Serial.println(httpCode);

  if (httpCode <= 0) {
    Serial.println("Anonymous sign-in request failed");
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  Serial.println("Firebase auth response:");
  Serial.println(response);

  firebaseIdToken = jsonQuotedValue(response, "idToken");
  firebaseRefreshToken = jsonQuotedValue(response, "refreshToken");
  firebaseLocalId = jsonQuotedValue(response, "localId");

  Serial.print("Parsed UID: ");
  Serial.println(firebaseLocalId);

  Serial.print("Parsed token length: ");
  Serial.println(firebaseIdToken.length());

  if (firebaseIdToken.length() == 0) {
    Serial.println("Failed to parse Firebase ID token");
    return false;
  }

  firebaseTokenTime = millis();
  Serial.println("Firebase anonymous sign-in OK");
  return true;
}

void ensureFirebaseAuth() {
  if (firebaseIdToken == "" || millis() - firebaseTokenTime > authRefreshInterval) {
    Serial.println("Refreshing Firebase auth...");
    firebaseAnonymousSignIn();
  }
}
