    const canvas = document.getElementById('starfield');
        const ctx = canvas.getContext('2d');

        function resizeCanvas() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
        }
        resizeCanvas();
        window.addEventListener('resize', resizeCanvas);

        // Star configuration
        const stars = [];
        const numStars = 150; // Change this number to add more or fewer stars

        // Generate properties for each individual star
        for (let i = 0; i < numStars; i++) {
            stars.push({
                x: Math.random() * canvas.width,
                y: Math.random() * canvas.height,
                radius: Math.random() * 1.5, // Random star size
                alpha: Math.random(),        // Initial brightness/opacity
                twinkleSpeed: 0.01 + Math.random() * 0.02, // Random speed of twinkling
                direction: Math.random() > 0.5 ? 1 : -1 // Brightening or dimming
            });
        }

        // The Animation Loop
        function animate() {
            // Clear the canvas for the next frame
            ctx.clearRect(0, 0, canvas.width, canvas.height);

            // Redraw each star
            stars.forEach(star => {
                // Update opacity for the twinkling effect
                star.alpha += star.twinkleSpeed * star.direction;

                // Reverse the fade direction if it gets too bright or dark
                if (star.alpha >= 1 || star.alpha <= 0.1) {
                    star.direction *= -1;
                }

                // Draw the star on the canvas
                ctx.beginPath();
                ctx.arc(star.x, star.y, star.radius, 0, Math.PI * 2);
                ctx.fillStyle = `rgba(255, 255, 255, ${star.alpha})`;
                ctx.fill();
            });
            requestAnimationFrame(animate);
        }
        animate();