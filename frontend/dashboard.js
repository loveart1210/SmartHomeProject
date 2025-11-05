window.onload = function() {
  const ESP32_IP = "http://172.20.10.14";

  async function api(path, opts = {}) {
    try {
      const res = await fetch(`${ESP32_IP}${path}`, opts);
      return await res.json();
    } catch (e) {
      console.error("API error:", e);
      return {};
    }
  }

  function updateUI(s) {
    const lightBtn = document.getElementById('btnLight');
    const lightState = document.getElementById('lightState');
    if (s.light) {
      lightBtn.className = 'on';
      lightState.textContent = 'BẬT';
    } else {
      lightBtn.className = 'off';
      lightState.textContent = 'TẮT';
    }

    const fanBtn = document.getElementById('btnFan');
    const fanState = document.getElementById('fanState');
    if (s.fan) {
      fanBtn.className = 'on';
      fanState.textContent = 'BẬT';
    } else {
      fanBtn.className = 'off';
      fanState.textContent = 'TẮT';
    }

    document.getElementById('mq2Val').textContent = s.mq2 ?? '--';
    document.getElementById('reedState').textContent = s.reed ? 'ĐÓNG' : 'MỞ';
    document.getElementById('irState').textContent = s.ir ? 'CÓ VẬT' : 'KHÔNG';
    document.getElementById('alarmState').textContent = s.alarm ? 'ĐANG BÁO ĐỘNG' : 'BÌNH THƯỜNG';
    document.getElementById('alarmState').className = 'status ' + (s.alarm ? 'danger' : 'ok');
    document.getElementById('doorState').textContent = s.door ? 'MỞ' : 'ĐÓNG';
  }

  async function refresh() {
    const s = await api('/api/status');
    updateUI(s);
  }

  document.getElementById('btnLight').onclick = async () => {
    const s = await api('/api/relay?ch=light', { method: 'POST' });
    updateUI(s);
  };

  document.getElementById('btnFan').onclick = async () => {
    const s = await api('/api/relay?ch=fan', { method: 'POST' });
    updateUI(s);
  };

  document.getElementById('btnOpenDoor').onclick = async () => {
    await api('/api/servo?angle=90', { method: 'POST' });
    refresh();
  };

  document.getElementById('btnCloseDoor').onclick = async () => {
    await api('/api/servo?angle=0', { method: 'POST' });
    refresh();
  };

  document.getElementById('btnResetAlarm').onclick = async () => {
    await api('/api/reset_alarm', { method: 'POST' });
    refresh();
  };

  let rec;
  function startVoice() {
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) { alert('Trình duyệt không hỗ trợ SpeechRecognition'); return; }
    if (rec) { rec.stop(); rec = null; }
    rec = new SpeechRecognition();
    rec.lang = 'vi-VN';
    rec.interimResults = false;
    rec.maxAlternatives = 1;
    rec.onresult = async (e) => {
      const txt = e.results[0][0].transcript.toLowerCase();
      document.getElementById('voiceTxt').textContent = txt;
      await fetch(`${ESP32_IP}/api/voice`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'cmd=' + encodeURIComponent(txt)
      });
      setTimeout(refresh, 500);
    };
    rec.onerror = (e) => console.log(e);
    rec.start();
  }

  document.getElementById('btnVoice').onclick = startVoice;

    // 🟩 Lấy thông tin người dùng đã đăng nhập
  const user = JSON.parse(localStorage.getItem("user"));
  if (!user) {
    alert("Bạn chưa đăng nhập!");
    window.location.href = "login.html";
    return;
  }

  // 🟩 Kiểm tra quyền điều khiển thiết bị
  if (!user.permission_light) {
    const lightBtn = document.getElementById("btnLight");
    lightBtn.disabled = true;
    lightBtn.title = "Bạn không có quyền điều khiển đèn";
    lightBtn.style.opacity = "0.5";
    lightBtn.style.cursor = "not-allowed";
  }

  if (!user.permission_fan) {
    const fanBtn = document.getElementById("btnFan");
    fanBtn.disabled = true;
    fanBtn.title = "Bạn không có quyền điều khiển quạt";
    fanBtn.style.opacity = "0.5";
    fanBtn.style.cursor = "not-allowed";
  }

  // 🟩 Admin được phép điều khiển toàn bộ
  if (user.role === "admin") {
    document.getElementById("btnLight").disabled = false;
    document.getElementById("btnFan").disabled = false;
  }

  // 🟥 Xử lý nút đăng xuất
  const btnLogout = document.getElementById("btnLogout");
  btnLogout.onclick = function () {
    if (confirm("Bạn có chắc chắn muốn đăng xuất không?")) {
      localStorage.removeItem("user");
      window.location.href = "login.html";
    }
  };

  setInterval(refresh, 1500);
  refresh();
};
