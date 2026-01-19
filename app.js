const TEXT = {
  fillLocalApi: "??????? localApiBase ? index.html",
  empty: "???? ?????",
  localApiMissing: "local api ?? ?????",
  fillEmailPassword: "??????? email ? ??????",
  signInOk: "???? ????????",
  accountCreated: "??????? ??????",
  signOutOk: "????? ????????",
  chooseFiles: "?????? ?????",
  done: "??????",
  refreshing: "????????",
  errorPrefix: "??????: ",
  listErrorPrefix: "?????? ??????: ",
  uploaded: "?????????",
  filesLabel: "??????"
};

const config = window.APP_CONFIG || {};
const localApiBase = (config.localApiBase || "").trim().replace(/\/$/, "");
const hasLocal = Boolean(localApiBase);

const authCard = document.getElementById("authCard");
const uploadCard = document.getElementById("uploadCard");
const galleryCard = document.getElementById("galleryCard");
const configCard = document.getElementById("configCard");
const authMessage = document.getElementById("authMessage");
const uploadStatus = document.getElementById("uploadStatus");
const galleryGrid = document.getElementById("galleryGrid");
const galleryCount = document.getElementById("galleryCount");
const signOutBtn = document.getElementById("signOutBtn");
const refreshBtn = document.getElementById("refreshBtn");
const uploadBtn = document.getElementById("uploadBtn");
const signInBtn = document.getElementById("signInBtn");
const signUpBtn = document.getElementById("signUpBtn");
const authForm = document.getElementById("authForm");
const emailInput = document.getElementById("emailInput");
const passwordInput = document.getElementById("passwordInput");
const fileInput = document.getElementById("fileInput");

let localToken = localStorage.getItem("localToken") || "";

if (!hasLocal) {
  configCard.hidden = false;
  setStatus(authMessage, TEXT.fillLocalApi, "error");
  disableControls();
} else {
  configCard.hidden = true;
}

function disableControls() {
  [signInBtn, signUpBtn, uploadBtn, refreshBtn, signOutBtn].forEach((btn) => {
    if (btn) {
      btn.disabled = true;
    }
  });
}

function setStatus(element, message, tone = "info") {
  if (!element) return;
  element.textContent = message;
  element.dataset.tone = tone;
}

function clearStatus() {
  setStatus(authMessage, "", "info");
  setStatus(uploadStatus, "", "info");
}

function sanitizeName(name) {
  return name.toLowerCase().replace(/[^a-z0-9._-]+/g, "_").slice(0, 60);
}

function isVideoFile(item) {
  const mime = item?.mime_type;
  if (mime) {
    return mime.startsWith("video/");
  }
  const name = item?.name || "";
  return /\.(mp4|webm|mov|m4v|avi)$/i.test(name);
}

function updateUi(signedIn) {
  authCard.hidden = signedIn;
  uploadCard.hidden = !signedIn;
  galleryCard.hidden = !signedIn;
  signOutBtn.disabled = !signedIn;
  signOutBtn.hidden = !signedIn;

  if (!signedIn) {
    galleryGrid.innerHTML = "";
    galleryCount.textContent = TEXT.empty;
  }
}

async function apiRequest(path, options = {}) {
  if (!localApiBase) {
    throw new Error(TEXT.localApiMissing);
  }

  const headers = new Headers(options.headers || {});
  const isForm = options.body instanceof FormData;

  if (!isForm && options.body && !headers.has("Content-Type")) {
    headers.set("Content-Type", "application/json");
  }
  if (localToken) {
    headers.set("Authorization", `Bearer ${localToken}`);
  }

  const response = await fetch(`${localApiBase}${path}`, {
    ...options,
    headers,
    body: isForm ? options.body : options.body ? JSON.stringify(options.body) : undefined
  });

  const contentType = response.headers.get("content-type") || "";
  const data = contentType.includes("application/json") ? await response.json().catch(() => ({})) : {};

  if (!response.ok) {
    const message = data.error || `http ${response.status}`;
    throw new Error(message);
  }

  return data;
}

async function signIn() {
  clearStatus();
  const email = emailInput.value.trim();
  const password = passwordInput.value;
  if (!email || !password) {
    setStatus(authMessage, TEXT.fillEmailPassword, "error");
    return;
  }
  try {
    const data = await apiRequest("/api/auth/signin", { method: "POST", body: { email, password } });
    localToken = data.token;
    localStorage.setItem("localToken", localToken);
    updateUi(true);
    setStatus(authMessage, TEXT.signInOk, "success");
    await refreshGallery();
  } catch (error) {
    setStatus(authMessage, `${TEXT.errorPrefix}${error.message}`, "error");
  }
}

async function signUp() {
  clearStatus();
  const email = emailInput.value.trim();
  const password = passwordInput.value;
  if (!email || !password) {
    setStatus(authMessage, TEXT.fillEmailPassword, "error");
    return;
  }
  try {
    const data = await apiRequest("/api/auth/signup", { method: "POST", body: { email, password } });
    localToken = data.token;
    localStorage.setItem("localToken", localToken);
    updateUi(true);
    setStatus(authMessage, TEXT.accountCreated, "success");
    await refreshGallery();
  } catch (error) {
    setStatus(authMessage, `${TEXT.errorPrefix}${error.message}`, "error");
  }
}

async function signOut() {
  clearStatus();
  try {
    await apiRequest("/api/auth/signout", { method: "POST" });
  } catch (error) {
    // ignore
  }
  localToken = "";
  localStorage.removeItem("localToken");
  updateUi(false);
  setStatus(authMessage, TEXT.signOutOk, "info");
}

async function uploadFiles() {
  clearStatus();
  const files = Array.from(fileInput.files || []);
  if (!files.length) {
    setStatus(uploadStatus, TEXT.chooseFiles, "error");
    return;
  }

  uploadBtn.disabled = true;
  let uploaded = 0;

  for (const file of files) {
    const formData = new FormData();
    formData.append("file", file);

    try {
      await apiRequest("/api/upload", { method: "POST", body: formData });
      uploaded += 1;
      setStatus(uploadStatus, `${TEXT.uploaded} ${uploaded} ?? ${files.length}`, "info");
    } catch (error) {
      setStatus(uploadStatus, `${TEXT.errorPrefix}${error.message}`, "error");
      uploadBtn.disabled = false;
      return;
    }
  }

  uploadBtn.disabled = false;
  fileInput.value = "";
  setStatus(uploadStatus, TEXT.done, "success");
  await refreshGallery();
}

async function refreshGallery() {
  try {
    setStatus(uploadStatus, TEXT.refreshing, "info");
    const data = await apiRequest("/api/files");
    const files = data.files || [];

    if (!files.length) {
      galleryGrid.innerHTML = "";
      galleryCount.textContent = TEXT.empty;
      setStatus(uploadStatus, "", "info");
      return;
    }

    galleryGrid.innerHTML = "";
    files.forEach((file) => {
      galleryGrid.appendChild(
        renderMediaItem({
          name: file.original_name || file.name,
          url: file.url,
          mime_type: file.mime_type
        })
      );
    });
    galleryCount.textContent = `${TEXT.filesLabel}: ${files.length}`;
    setStatus(uploadStatus, "", "info");
  } catch (error) {
    setStatus(uploadStatus, `${TEXT.listErrorPrefix}${error.message}`, "error");
  }
}

function renderMediaItem(item) {
  const wrapper = document.createElement("article");
  wrapper.className = "media-item";

  const media = isVideoFile(item) ? document.createElement("video") : document.createElement("img");
  if (media.tagName === "VIDEO") {
    media.controls = true;
    media.playsInline = true;
    media.preload = "metadata";
  } else {
    media.loading = "lazy";
    media.alt = item.name;
  }
  media.src = item.url;

  const caption = document.createElement("div");
  caption.className = "media-meta";
  caption.textContent = item.name;

  wrapper.appendChild(media);
  wrapper.appendChild(caption);
  return wrapper;
}

async function checkLocalSession() {
  if (!localToken) {
    updateUi(false);
    return;
  }
  try {
    await apiRequest("/api/auth/me");
    updateUi(true);
    await refreshGallery();
  } catch (error) {
    localToken = "";
    localStorage.removeItem("localToken");
    updateUi(false);
  }
}

async function init() {
  if (hasLocal) {
    await checkLocalSession();
  }
}

if (hasLocal) {
  authForm.addEventListener("submit", (event) => {
    event.preventDefault();
    signIn();
  });

  signInBtn.addEventListener("click", signIn);
  signUpBtn.addEventListener("click", signUp);
  signOutBtn.addEventListener("click", signOut);
  uploadBtn.addEventListener("click", uploadFiles);
  refreshBtn.addEventListener("click", refreshGallery);
}

init();
