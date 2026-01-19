import { createClient } from "https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm";

const config = window.APP_CONFIG || {};
const supabaseUrl = (config.supabaseUrl || "").trim();
const supabaseAnonKey = (config.supabaseAnonKey || "").trim();
const bucket = (config.bucket || "media").trim();

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

const hasConfig = Boolean(supabaseUrl && supabaseAnonKey);
if (!hasConfig) {
  configCard.hidden = false;
  authCard.dataset.locked = "true";
  setStatus(authMessage, "??????? APP_CONFIG ? index.html", "error");
  disableControls();
} else {
  configCard.hidden = true;
}

const supabase = hasConfig ? createClient(supabaseUrl, supabaseAnonKey) : null;

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

function sanitizeName(name) {
  return name.toLowerCase().replace(/[^a-z0-9._-]+/g, "_").slice(0, 60);
}

function isVideoFile(file) {
  if (file.metadata && file.metadata.mimetype) {
    return file.metadata.mimetype.startsWith("video/");
  }
  return /\.(mp4|webm|mov|m4v|avi)$/i.test(file.name || "");
}

function updateUi(session) {
  const signedIn = Boolean(session);
  authCard.hidden = signedIn;
  uploadCard.hidden = !signedIn;
  galleryCard.hidden = !signedIn;
  signOutBtn.disabled = !signedIn;
  signOutBtn.hidden = !signedIn;

  if (!signedIn) {
    galleryGrid.innerHTML = "";
    galleryCount.textContent = "???? ?????";
  }
}

async function signIn() {
  clearStatus();
  const email = emailInput.value.trim();
  const password = passwordInput.value;
  if (!email || !password) {
    setStatus(authMessage, "??????? email ? ??????", "error");
    return;
  }
  const { error } = await supabase.auth.signInWithPassword({ email, password });
  if (error) {
    setStatus(authMessage, error.message.toLowerCase(), "error");
    return;
  }
  setStatus(authMessage, "???? ????????", "success");
}

async function signUp() {
  clearStatus();
  const email = emailInput.value.trim();
  const password = passwordInput.value;
  if (!email || !password) {
    setStatus(authMessage, "??????? email ? ??????", "error");
    return;
  }
  const { data, error } = await supabase.auth.signUp({ email, password });
  if (error) {
    setStatus(authMessage, error.message.toLowerCase(), "error");
    return;
  }
  if (data.session) {
    setStatus(authMessage, "??????? ??????", "success");
  } else {
    setStatus(authMessage, "??????? ????? ??? ?????????????", "info");
  }
}

async function signOut() {
  clearStatus();
  await supabase.auth.signOut();
  setStatus(authMessage, "????? ????????", "info");
}

function clearStatus() {
  setStatus(authMessage, "", "info");
  setStatus(uploadStatus, "", "info");
}

async function uploadFiles() {
  clearStatus();
  const files = Array.from(fileInput.files || []);
  if (!files.length) {
    setStatus(uploadStatus, "?????? ?????", "error");
    return;
  }

  uploadBtn.disabled = true;
  let uploaded = 0;

  for (const file of files) {
    const fileName = sanitizeName(file.name || "file");
    const uniqueId = crypto.randomUUID ? crypto.randomUUID() : String(Date.now());
    const filePath = `shared/${uniqueId}-${fileName}`;

    const { error } = await supabase.storage
      .from(bucket)
      .upload(filePath, file, { cacheControl: "3600", upsert: false, contentType: file.type });

    if (error) {
      setStatus(uploadStatus, `??????: ${error.message.toLowerCase()}`, "error");
      uploadBtn.disabled = false;
      return;
    }
    uploaded += 1;
    setStatus(uploadStatus, `????????? ${uploaded} ?? ${files.length}`, "info");
  }

  uploadBtn.disabled = false;
  fileInput.value = "";
  setStatus(uploadStatus, "??????", "success");
  await refreshGallery();
}

async function refreshGallery() {
  if (!galleryGrid) return;
  setStatus(uploadStatus, "????????", "info");

  const { data, error } = await supabase.storage
    .from(bucket)
    .list("shared", { limit: 100, sortBy: { column: "created_at", order: "desc" } });

  if (error) {
    setStatus(uploadStatus, `?????? ??????: ${error.message.toLowerCase()}`, "error");
    return;
  }

  if (!data || data.length === 0) {
    galleryGrid.innerHTML = "";
    galleryCount.textContent = "???? ?????";
    setStatus(uploadStatus, "", "info");
    return;
  }

  galleryGrid.innerHTML = "";

  const items = await Promise.all(
    data.map(async (file) => {
      const path = `shared/${file.name}`;
      const { data: urlData, error: urlError } = await supabase.storage
        .from(bucket)
        .createSignedUrl(path, 60 * 60);
      if (urlError) {
        return null;
      }
      return {
        name: file.name,
        url: urlData.signedUrl,
        meta: file
      };
    })
  );

  const filtered = items.filter(Boolean);
  filtered.forEach((item) => galleryGrid.appendChild(renderMediaItem(item)));
  galleryCount.textContent = `??????: ${filtered.length}`;
  setStatus(uploadStatus, "", "info");
}

function renderMediaItem(item) {
  const wrapper = document.createElement("article");
  wrapper.className = "media-item";

  const media = isVideoFile(item.meta) ? document.createElement("video") : document.createElement("img");
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

if (hasConfig) {
  const {
    data: { session }
  } = await supabase.auth.getSession();
  updateUi(session);
  if (session) {
    refreshGallery();
  }

  supabase.auth.onAuthStateChange((_event, updatedSession) => {
    updateUi(updatedSession);
    if (updatedSession) {
      refreshGallery();
    }
  });

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
