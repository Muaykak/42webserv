    const canvas = document.getElementById('starfield');
        const ctx = canvas.getContext('2d');

        const stars = [];
        const numStars = 150;

        for (let i = 0; i < numStars; i++) {
            stars.push({
                x: Math.random() * canvas.width,
                y: Math.random() * canvas.height,
                radius: Math.random() * 1.5,
                alpha: Math.random(),
                blinkSpeed: 0.01 + Math.random() * 0.03,
                direction: Math.random() > 0.5 ? 1 : -1
            });
        }

        function animate() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            stars.forEach(star => {
                star.alpha += star.blinkSpeed * star.direction;
                if (star.alpha >= 1 || star.alpha <= 0.1) {
                    star.direction *= -1;
                }

                ctx.beginPath();
                ctx.arc(star.x, star.y, star.radius, 0, Math.PI * 2);
                ctx.fillStyle = `rgba(255, 255, 255, ${star.alpha})`;
                ctx.fill();
            });
            requestAnimationFrame(animate);
        }
        animate();